#include <iostream>
#include <memory>
#include <cstdlib>

#include "buzzweb/domain/InMemoryRoomRepository.hpp"
#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/net/Server.hpp"
#include "util/Config.hpp"

int main () {
    buzzweb::net::ServerConfig config;

    std::cout << "[INFO] Loading config..." << std::endl;
    try {
        config = LoadConfig();
    } catch (const std::exception & ex) {
        std::cerr << "[ERROR] Couldn't load config: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "[INFO] Config loaded!" << std::endl;

    std::cout << "[INFO] Starting BuzzWeb Server..." << std::endl;

    auto repository = std::make_shared<buzzweb::domain::InMemoryRoomRepository>();
    buzzweb::app::RoomService room_service(repository);
    buzzweb::app::ControlDispatcher dispatcher(room_service);
    buzzweb::net::Server server(config, dispatcher);
    server.Run();

    return 0;
}