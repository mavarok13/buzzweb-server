#pragma once

#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/domain/Participant.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <memory>
#include <string>

namespace buzzweb::net {

class SessionRegistry;

using TcpSocket = boost::asio::ip::tcp::socket;
using TlsStream = boost::asio::ssl::stream<TcpSocket>;
using WebSocketStream = boost::beast::websocket::stream<TlsStream>;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(
        TcpSocket socket,
        boost::asio::ssl::context& tls_context,
        app::ControlDispatcher& dispatcher,
        SessionRegistry& registry
    );
    ~Session();

    const domain::ParticipantId& ParticipantId() const;
    void Start();
    void Send(std::string message);
    void Close();

private:
    void OnTlsHandshake();
    void OnWebSocketAccept();
    void ReadNext();
    void HandleMessage(std::string message);

    WebSocketStream websocket_;
    boost::beast::flat_buffer buffer_;
    app::ControlDispatcher& dispatcher_;
    SessionRegistry& registry_;
    domain::ParticipantId participant_id_;
};

using SessionPtr = std::shared_ptr<Session>;

} // namespace buzzweb::net
