#include <iostream>
#include <memory>
#include <cstdlib>
#include <stdexcept>
#include <mutex>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include "buzzweb/domain/InMemoryRoomRepository.hpp"
#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/net/Server.hpp"
#include "util/Config.hpp"

void ConfigureLogging() {
    static std::once_flag configured;

    std::call_once(configured, [] {
        boost::log::add_console_log(std::clog);
        boost::log::add_common_attributes();

        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= boost::log::trivial::info
        );
    });
}

int main () {
    ConfigureLogging();

    buzzweb::net::ServerConfig config;

    BOOST_LOG_TRIVIAL(info) << "Loading config...";
    try {
        config = LoadConfig();
    } catch (const std::exception & ex) {
        BOOST_LOG_TRIVIAL(error) << "Couldn't load config: " << ex.what();
        return EXIT_FAILURE;
    }
    BOOST_LOG_TRIVIAL(info) << "Config loaded!";

    BOOST_LOG_TRIVIAL(info) << "Starting BuzzWeb Server...";

    auto repository = std::make_shared<buzzweb::domain::InMemoryRoomRepository>();
    buzzweb::app::RoomService room_service(repository);
    buzzweb::net::SessionRegistry registry;
    buzzweb::net::Server server(config, registry, room_service);
    server.Run();

    return 0;
}
