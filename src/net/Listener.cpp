#include "buzzweb/net/Listener.hpp"

#include "buzzweb/net/Session.hpp"

#include <boost/asio/socket_base.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/system_error.hpp>
#include <memory>
#include <utility>

namespace buzzweb::net {

Listener::Listener(
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context,
    boost::asio::ip::tcp::endpoint endpoint,
    bool tls_enabled,
    app::ControlDispatcher& dispatcher,
    SessionRegistry& registry
)
    : io_context_(io_context),
      tls_context_(tls_context),
      acceptor_(io_context),
      tls_enabled_(tls_enabled),
      dispatcher_(dispatcher),
      registry_(registry)
{
    boost::system::error_code error;
    acceptor_.open(endpoint.protocol(), error);
    if (error) {
        throw boost::system::system_error(error);
    }

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), error);
    if (error) {
        throw boost::system::system_error(error);
    }

    acceptor_.bind(endpoint, error);
    if (error) {
        throw boost::system::system_error(error);
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, error);
    if (error) {
        throw boost::system::system_error(error);
    }
}

Listener::~Listener() = default;

void Listener::Start()
{
    AcceptNext();
}

void Listener::Stop()
{
    boost::system::error_code ignored;
    acceptor_.close(ignored);
}

void Listener::AcceptNext()
{
    acceptor_.async_accept(
        [this](boost::system::error_code error, boost::asio::ip::tcp::socket socket) {
            OnAccept(error, std::move(socket));
        }
    );
}

void Listener::OnAccept(boost::system::error_code error, boost::asio::ip::tcp::socket socket)
{
    if (!acceptor_.is_open()) {
        return;
    }

    if (error) {
        BOOST_LOG_TRIVIAL(error) << "TCP accept failed: " << error.message();
        AcceptNext();
        return;
    }

    if (tls_enabled_) {
        std::make_shared<SslSession>(std::move(socket), tls_context_, dispatcher_, registry_)->Start();
    } else {
        std::make_shared<PlainSession>(std::move(socket), dispatcher_, registry_)->Start();
    }
    AcceptNext();
}

} // namespace buzzweb::net
