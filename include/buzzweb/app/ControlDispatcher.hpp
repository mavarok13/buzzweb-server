#pragma once

#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/domain/Participant.hpp"
#include "buzzweb/domain/Room.hpp"

#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace buzzweb::app {

enum ControlMessageType {
    CreateRoom,
    JoinRoom,
    LeaveRoom,
    GetRoomParticipants
};

struct ControlMessage {
    ControlMessageType type;
    std::optional<std::string> request_id;
    nlohmann::json payload;
};

struct ControlError {
    std::string code;
    std::string message;
};

struct ControlResponse {
    ControlMessageType type;
    std::optional<std::string> request_id;
    bool ok;
    nlohmann::json payload;
    std::optional<ControlError> error;
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
