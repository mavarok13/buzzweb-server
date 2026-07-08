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
    GetRoomParticipants,
    JoinEvent,
    LeftEvent
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

struct ControlEventData {
public:
    ControlMessageType type;
    domain::RoomCode room_code;
    domain::ParticipantId participant_id;
};

using ControlSendHandler = std::function<void(const ControlResponse& response)>;
using ControlEventHandler = std::function<void(domain::RoomCode room_code, domain::ParticipantId participant_id, const std::string& event_message)>;

class ControlDispatcher {
public:
    explicit ControlDispatcher(RoomService& room_service, ControlEventHandler event_handler);
    ~ControlDispatcher();

    void Dispatch(
        const domain::ParticipantId& participant_id,
        const ControlMessage& message,
        ControlSendHandler send
    );

private:
    RoomService& room_service_;
    ControlEventHandler event_handler_;
};

} // namespace buzzweb::app
