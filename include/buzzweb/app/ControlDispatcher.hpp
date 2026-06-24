#pragma once

#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/domain/Participant.hpp"
#include "buzzweb/domain/Room.hpp"

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

namespace buzzweb::app {

struct ControlMessage {
    std::string type;
    nlohmann::json payload;
};

struct ControlResponse {
    std::string type;
    nlohmann::json payload;
};

using ControlSendHandler = std::function<void(const ControlResponse& response)>;

class ControlDispatcher {
public:
    explicit ControlDispatcher(RoomService& room_service);
    ~ControlDispatcher();

    void Dispatch(
        const domain::ParticipantId& participant_id,
        const ControlMessage& message,
        ControlSendHandler send
    );

private:
    RoomService& room_service_;
};

} // namespace buzzweb::app
