#include <iostream>
#include <memory>
#include <cstdlib>

#include "buzzweb/domain/InMemoryRoomRepository.hpp"
#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/net/Server.hpp"
#include "buzzweb/net/Session.hpp"
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
    buzzweb::net::SessionRegistry registry;
    buzzweb::app::ControlDispatcher dispatcher(room_service, [&room_service, &registry] (buzzweb::domain::RoomCode room_code, buzzweb::domain::ParticipantId participant_id, const std::string & event_message) {
        const auto participants = room_service.GetRoomParticipants(room_code);
        for (const auto participant : participants) {
            if (participant.GetId() == participant_id) {
                continue;
            }

            auto session_ptr = registry.Find(participant.GetId());
            if (session_ptr) {
                (*session_ptr)->Send(event_message);
            }
        }
    });
    buzzweb::net::Server server(config, registry, dispatcher);
    server.Run();

    return 0;
}
