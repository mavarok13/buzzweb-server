#include "buzzweb/app/ControlDispatcher.hpp"

#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace buzzweb::app {
namespace {

nlohmann::json ParticipantToJson(const domain::Participant& participant)
{
    return {
        {"participant_id", participant.GetId()},
        {"display_name", participant.GetName()}
    };
}

nlohmann::json ParticipantsToJson(const std::vector<domain::Participant>& participants)
{
    auto result = nlohmann::json::array();
    for (const auto& participant : participants) {
        result.push_back(ParticipantToJson(participant));
    }

    return result;
}

std::optional<std::string> OptionalString(const nlohmann::json& payload, const char* field)
{
    if (!payload.contains(field) || payload.at(field).is_null()) {
        return std::nullopt;
    }

    if (!payload.at(field).is_string()) {
        throw std::invalid_argument("invalid_field");
    }

    return payload.at(field).get<std::string>();
}

std::string RequiredString(const nlohmann::json& payload, const char* field)
{
    if (!payload.contains(field)) {
        throw std::invalid_argument("missing_field");
    }

    if (!payload.at(field).is_string()) {
        throw std::invalid_argument("invalid_field");
    }

    return payload.at(field).get<std::string>();
}

std::string ErrorMessage(const std::string& code)
{
    if (code == "missing_field") {
        return "Required field is missing.";
    }
    if (code == "invalid_field") {
        return "Field has an invalid value.";
    }
    if (code == "invalid_message") {
        return "Message type or payload is invalid.";
    }
    if (code == "room_not_found") {
        return "Room was not found.";
    }
    if (code == "wrong_password") {
        return "Room password is incorrect.";
    }
    if (code == "already_joined") {
        return "Participant is already in the room.";
    }
    if (code == "not_in_room") {
        return "Participant is not in the room.";
    }

    return "Internal server error.";
}

ControlResponse SuccessResponse(
    ControlMessageType type,
    std::optional<std::string> request_id,
    nlohmann::json payload
)
{
    return {std::move(type), std::move(request_id), true, std::move(payload), std::nullopt};
}

ControlResponse ErrorResponse(ControlMessageType type, std::optional<std::string> request_id, std::string code)
{
    auto message = ErrorMessage(code);

    return {
        std::move(type),
        std::move(request_id),
        false,
        nlohmann::json::object(),
        ControlError{std::move(code), std::move(message)}
    };
}

} // namespace

ControlDispatcher::ControlDispatcher(RoomService& room_service)
    : room_service_(room_service)
{
}

ControlDispatcher::~ControlDispatcher() = default;

void ControlDispatcher::Dispatch(
    const domain::ParticipantId& participant_id,
    const ControlMessage& message,
    ControlSendHandler send
)
{
    if (!send) {
        return;
    }

    const auto response_type = message.type;

    try {
        if (!message.payload.is_object()) {
            send(ErrorResponse(response_type, message.request_id, "invalid_message"));
            return;
        }

        if (message.type == ControlMessageType::CreateRoom) {
            auto display_name = RequiredString(message.payload, "display_name");
            auto password = OptionalString(message.payload, "password");
            auto room = room_service_.CreateRoom(password);
            domain::Participant participant(participant_id, std::move(display_name));
            auto joined_room = room_service_.JoinRoom(room.GetCode(), participant, password);

            send(SuccessResponse(
                response_type,
                message.request_id,
                {
                    {"room_code", joined_room.GetCode()},
                    {"participant", ParticipantToJson(participant)},
                    {"participants", ParticipantsToJson(joined_room.GetParticipants())}
                }
            ));
            return;
        }

        if (message.type == ControlMessageType::JoinRoom) {
            auto room_code = RequiredString(message.payload, "room_code");
            auto display_name = RequiredString(message.payload, "display_name");
            auto password = OptionalString(message.payload, "password");
            domain::Participant participant(participant_id, std::move(display_name));
            auto room = room_service_.JoinRoom(room_code, participant, std::move(password));

            send(SuccessResponse(
                response_type,
                message.request_id,
                {
                    {"room_code", room.GetCode()},
                    {"participant", ParticipantToJson(participant)},
                    {"participants", ParticipantsToJson(room.GetParticipants())}
                }
            ));
            return;
        }

        if (message.type == ControlMessageType::LeaveRoom) {
            auto room_code = RequiredString(message.payload, "room_code");
            room_service_.LeaveRoom(room_code, participant_id);

            send(SuccessResponse(response_type, message.request_id, {{"room_code", room_code}}));
            return;
        }

        send(ErrorResponse(response_type, message.request_id, "invalid_message"));
    } catch (const std::invalid_argument& error) {
        send(ErrorResponse(response_type, message.request_id, error.what()));
    } catch (const std::runtime_error& error) {
        send(ErrorResponse(response_type, message.request_id, error.what()));
    } catch (const std::exception&) {
        send(ErrorResponse(response_type, message.request_id, "internal_error"));
    }
}

} // namespace buzzweb::app
