#pragma once

#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/net/SessionRegistry.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/system/error_code.hpp>

namespace buzzweb::net {

class Listener {
public:
    Listener(
        boost::asio::io_context& io_context,
        boost::asio::ssl::context& tls_context,
        boost::asio::ip::tcp::endpoint endpoint,
        app::ControlDispatcher& dispatcher,
        SessionRegistry& registry
    );
    ~Listener();

    void Start();
    void Stop();

private:
    void AcceptNext();
    void OnAccept(boost::system::error_code error, boost::asio::ip::tcp::socket socket);

    boost::asio::io_context& io_context_;
    boost::asio::ssl::context& tls_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    app::ControlDispatcher& dispatcher_;
    SessionRegistry& registry_;
};

} // namespace buzzweb::net
