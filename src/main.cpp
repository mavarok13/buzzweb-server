#include <iostream>
#include <memory>
#include <cstdlib>
#include <stdexcept>

#include <nlohmann/json.hpp>

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
    buzzweb::app::ControlDispatcher dispatcher(room_service, [&registry] (const buzzweb::app::ControlEventData& event, const std::vector<buzzweb::domain::Participant>& participants) {
        for (const auto participant : participants) {
            if (participant.GetId() == event.sender_participant_id) {
                continue;
            }

            auto session_ptr_opt = registry.Find(participant.GetId());
            if (session_ptr_opt) {

                std::string event_type;
                if (event.type == buzzweb::app::ControlEventType::Joined) {
                    event_type = "participant_joined";
                } else if (event.type == buzzweb::app::ControlEventType::Left) {
                    event_type = "participant_left";
                } else {
                    throw std::runtime_error("invalid_event_type");
                }

                nlohmann::json event_message = {
                    {"type", event_type},
                    {"payload", event.payload}
                };

                (*session_ptr_opt)->Send(event_message.dump());
            }
        }
    });
    buzzweb::net::Server server(config, registry, dispatcher);
    server.Run();

    return 0;
}
