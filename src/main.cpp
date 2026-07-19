#include <iostream>
#include <memory>
#include <cstdlib>
#include <stdexcept>
#include <mutex>

#include <nlohmann/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include "buzzweb/domain/InMemoryRoomRepository.hpp"
#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/app/ControlDispatcher.hpp"
#include "buzzweb/net/Server.hpp"
#include "buzzweb/net/Session.hpp"
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
