#include "buzzweb/net/Server.hpp"

#include <boost/log/trivial.hpp>
#include <memory>
#include <utility>

namespace buzzweb::net {

Server::Server(ServerConfig config, net::SessionRegistry& registry, app::RoomService& room_service)
    : config_(std::move(config)),
      io_context_(1),
      tls_context_(boost::asio::ssl::context::tlsv12_server),
      registry_(registry),
      room_service_(room_service),
      dispatcher_(room_service_)
{
    if (config_.tls_enabled) {
        ConfigureTls();
    }

    listener_ = std::make_unique<Listener>(
        io_context_,
        tls_context_,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config_.port),
        config_.tls_enabled,
        dispatcher_,
        registry_
    );
}

Server::~Server() = default;

void Server::Run()
{
    listener_->Start();
    BOOST_LOG_TRIVIAL(info) << (config_.tls_enabled ? "WSS" : "WS") << " server listening on port " << config_.port;
    io_context_.run();
}

void Server::Stop()
{
    if (listener_) {
        listener_->Stop();
    }

    io_context_.stop();
}

void Server::ConfigureTls()
{
    tls_context_.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use
    );
    tls_context_.use_certificate_chain_file(config_.certificate_file);
    tls_context_.use_private_key_file(config_.private_key_file, boost::asio::ssl::context::pem);
}

} // namespace buzzweb::net
