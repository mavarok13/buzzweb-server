#pragma once

#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/net/Listener.hpp"
#include "buzzweb/net/SessionRegistry.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <memory>
#include <string>

namespace buzzweb::net {

struct ServerConfig {
    unsigned short port;
    std::string certificate_file;
    std::string private_key_file;
};

class Server {
public:
    Server(ServerConfig config, net::SessionRegistry& registry, app::ControlDispatcher& dispatcher);
    ~Server();

    void Run();
    void Stop();

private:
    void ConfigureTls();
    void ConfigureLogging();

    ServerConfig config_;
    boost::asio::io_context io_context_;
    boost::asio::ssl::context tls_context_;
    SessionRegistry& registry_;
    app::ControlDispatcher& dispatcher_;
    std::unique_ptr<Listener> listener_;
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_;
};

} // namespace buzzweb::net
