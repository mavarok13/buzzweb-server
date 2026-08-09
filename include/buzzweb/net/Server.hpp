#pragma once

#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/net/Listener.hpp"
#include "buzzweb/net/SessionRegistry.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <memory>
#include <string>

namespace buzzweb::net {

struct ServerConfig {
    unsigned short port;
    std::string certificate_file;
    std::string private_key_file;
    bool tls_enabled;
};

class Server {
public:
    Server(ServerConfig config, net::SessionRegistry& registry, app::RoomService& room_service);
    ~Server();

    void Run();
    void Stop();

private:
    void ConfigureTls();

    ServerConfig config_;
    boost::asio::io_context io_context_;
    boost::asio::ssl::context tls_context_;
    SessionRegistry& registry_;
    app::RoomService& room_service_;
    app::ControlDispatcher dispatcher_;
    std::unique_ptr<Listener> listener_;
};

} // namespace buzzweb::net
