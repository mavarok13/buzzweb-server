#pragma once

#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/domain/Participant.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace buzzweb::net {

class SessionRegistry;

using TcpSocket = boost::asio::ip::tcp::socket;
using TlsStream = boost::asio::ssl::stream<TcpSocket>;

using PlainWebSocketStream = boost::beast::websocket::stream<TcpSocket>;
using SslWebSocketStream = boost::beast::websocket::stream<TlsStream>;

class SessionBase : public std::enable_shared_from_this<SessionBase> {
public:
    virtual ~SessionBase() = default;

    const domain::ParticipantId& GetParticipantId() const;
    virtual void Start() = 0;
    virtual void Send(std::string message) = 0;
    virtual void Close() = 0;

protected:
    SessionBase(app::ControlDispatcher& dispatcher, SessionRegistry& registry);

    void RemoveFromRegistry();
    void HandleMessage(std::string message);
    void DeliverEvents(const std::vector<app::ControlDispatcherEvent>& events);
    std::string InvalidMessageError() const;

    app::ControlDispatcher& dispatcher_;
    SessionRegistry& registry_;
    domain::ParticipantId participant_id_;
    bool registered_ = false;
    bool closing_ = false;
};

template <typename WebSocketStream>
class Session : public SessionBase {
public:
    void Send(std::string message) override {
        auto self = std::static_pointer_cast<Session<WebSocketStream>>(shared_from_this());
        boost::asio::post(
            websocket_.get_executor(),
            [self, message = std::move(message)]() mutable {
                const bool writing = !self->write_queue_.empty();
                self->write_queue_.push_back(std::move(message));
                if (!writing) {
                    self->DoWrite();
                }
            }
        );
    }
    void Close() override {
        auto self = std::static_pointer_cast<Session<WebSocketStream>>(shared_from_this());
        boost::asio::post(
            websocket_.get_executor(),
            [self]() {
                if (self->closing_) {
                    return;
                }

                self->closing_ = true;
                self->RemoveFromRegistry();
                if (self->websocket_.is_open()) {
                    self->websocket_.async_close(
                        boost::beast::websocket::close_code::normal,
                        [self](boost::system::error_code) {}
                    );
                    return;
                }

                boost::system::error_code ignored;
                self->CloseLowestLayer(ignored);
            }
        );
    }

protected:
    Session(WebSocketStream websocket, app::ControlDispatcher& dispatcher, SessionRegistry& registry)
    : SessionBase(dispatcher, registry), websocket_(std::move(websocket)) {}

    virtual void CloseLowestLayer(boost::system::error_code& error) = 0;

    void AcceptWebSocket() {
        websocket_.text(true);
        websocket_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

        auto self = std::static_pointer_cast<Session<WebSocketStream>>(shared_from_this());
        websocket_.async_accept(
            [self](boost::system::error_code accept_error) {
                self->OnWebSocketAccept(accept_error);
            }
        );
    }
    void OnWebSocketAccept(boost::system::error_code error) {
        if (error) {
            Fail(error);
            return;
        }

        registry_.Add(shared_from_this());
        registered_ = true;
        ReadNext();
    }
    void ReadNext() {
        auto self = std::static_pointer_cast<Session<WebSocketStream>>(shared_from_this());
        websocket_.async_read(
            buffer_,
            [self](boost::system::error_code error, std::size_t bytes_transferred) {
                self->OnRead(error, bytes_transferred);
            }
        );
    }
    void OnRead(boost::system::error_code error, std::size_t bytes_transferred) {
        static_cast<void>(bytes_transferred);

        if (error == boost::beast::websocket::error::closed) {
            RemoveFromRegistry();
            return;
        }
        if (error) {
            Fail(error);
            return;
        }

        auto message = boost::beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        if (!websocket_.got_text()) {
            Send(InvalidMessageError());
            ReadNext();
            return;
        }

        HandleMessage(std::move(message));
        ReadNext();
    }
    void DoWrite() {
        if (write_queue_.empty() || closing_) {
            return;
        }

        auto self = std::static_pointer_cast<Session<WebSocketStream>>(shared_from_this());
        websocket_.async_write(
            boost::asio::buffer(write_queue_.front()),
            [self](boost::system::error_code error, std::size_t bytes_transferred) {
                static_cast<void>(bytes_transferred);

                if (error) {
                    self->Fail(error);
                    return;
                }

                self->write_queue_.pop_front();
                if (!self->write_queue_.empty()) {
                    self->DoWrite();
                }
            }
        );
    }
    void Fail(boost::system::error_code error) {
        if (error) {
            BOOST_LOG_TRIVIAL(error) << "Session " << participant_id_ << " failed: " << error.message();
        }

        closing_ = true;
        RemoveFromRegistry();

        boost::system::error_code ignored;
        CloseLowestLayer(ignored);
    }

    WebSocketStream websocket_;
    boost::beast::flat_buffer buffer_;
    std::deque<std::string> write_queue_;
};

class PlainSession final : public Session<PlainWebSocketStream> {
public:
    PlainSession(TcpSocket socket, app::ControlDispatcher& dispatcher, SessionRegistry& registry);

    void Start() override;
    void CloseLowestLayer(boost::system::error_code& error) override;
};

class SslSession final : public Session<SslWebSocketStream> {
public:
    SslSession(
        TcpSocket socket,
        boost::asio::ssl::context& tls_context,
        app::ControlDispatcher& dispatcher,
        SessionRegistry& registry
    );

    void Start() override;
    void OnTlsHandshake(boost::system::error_code error);
    void CloseLowestLayer(boost::system::error_code& error) override;
};

using SessionPtr = std::shared_ptr<SessionBase>;

} // namespace buzzweb::net
