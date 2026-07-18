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

std::string RoomServiceErrorToCode(const ErrorType& type) {
    switch(type) {
        case ErrorType::RoomNotFound: {
            return "room_not_found";
        };
        case ErrorType::WrongPassword: {
            return "wrong_password";
        };
        case ErrorType::AlreadyJoined: {
            return "already_joined";
        };
        case ErrorType::NotInRoom: {
            return "not_in_room";
        };
        case ErrorType::ParticipantUnavailable: {
            return "participant_unavailable";
        };
        default: {
            return "internal_error";
        };
    }
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
    if (code == "participant_unavailable") {
        return "Participant is not connected.";
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

ControlEventData BuildControlJoinedEventData(const domain::RoomCode& room_code, const domain::Participant& participant, const Participants& participants) {
    nlohmann::json payload = {
        {"room_code", room_code},
        {"participant", ParticipantToJson(participant)},
        {"participants", ParticipantsToJson(participants)}
    };

    return {ControlEventType::Joined, room_code, participant.GetId(), payload};
}

ControlEventData BuildControlLeftEventData(const domain::RoomCode& room_code, const domain::ParticipantId& participant_id, const Participants& participants) {
    nlohmann::json payload = {
        {"room_code", room_code},
        {"participant_id", participant_id},
        {"participants", ParticipantsToJson(participants)}
    };

    return {ControlEventType::Left, room_code, participant_id, payload};
}

} // namespace

ControlDispatcher::ControlDispatcher(RoomService& room_service, ControlEventHandler event_handler)
    : room_service_(room_service), event_handler_(std::move(event_handler))
{
}

ControlDispatcher::~ControlDispatcher() = default;

void ControlDispatcher::Dispatch(
    const domain::ParticipantId& participant_id,
    const ControlMessage& message,
    ControlSendHandler send,
    ControlRelayHandler relay
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
            auto create_room_result = room_service_.CreateRoom(password);

            if (!create_room_result.IsOk()) {
                std::string code = RoomServiceErrorToCode(create_room_result.Error().type);

                send(
                    ErrorResponse(
                        response_type,
                        message.request_id,
                        code
                    )
                );

                return;
            }

            domain::Room room = create_room_result.Value();

            domain::Participant participant(participant_id, std::move(display_name));
            auto joined_room_result = room_service_.JoinRoom(room.GetCode(), participant, password);

            if (joined_room_result.IsOk()) {
                auto joined_room = joined_room_result.Value();
                send(SuccessResponse(
                        response_type,
                        message.request_id,
                        {
                            {"room_code", joined_room.room_code},
                            {"participant", ParticipantToJson(participant)},
                            {"participants", ParticipantsToJson(joined_room.room_participants)}
                        }
                    )
                );
            } else {
                std::string code = RoomServiceErrorToCode(joined_room_result.Error().type);

                send(
                    ErrorResponse(
                        response_type,
                        message.request_id,
                        code
                    )
                );

                return;
            }
            
            return;
        }

        if (message.type == ControlMessageType::JoinRoom) {
            auto room_code = RequiredString(message.payload, "room_code");
            auto display_name = RequiredString(message.payload, "display_name");
            auto password = OptionalString(message.payload, "password");
            domain::Participant participant(participant_id, std::move(display_name));
            auto join_room_result = room_service_.JoinRoom(room_code, participant, std::move(password));

            if (join_room_result.IsOk()) {
                auto join_room = join_room_result.Value();

                send(SuccessResponse(
                        response_type,
                        message.request_id,
                        {
                            {"room_code", room_code},
                            {"participant", ParticipantToJson(participant)},
                            {"participants", ParticipantsToJson(join_room.room_participants)}
                        }
                    )
                );
                event_handler_(
                    BuildControlJoinedEventData(room_code, participant, join_room.room_participants),
                    join_room.room_participants
                );
            } else {
                std::string code = RoomServiceErrorToCode(join_room_result.Error().type);

                send(ErrorResponse(
                        response_type,
                        message.request_id,
                        code
                    )
                );

                return;
            }
            
            return;
        }

        if (message.type == ControlMessageType::LeaveRoom) {
            auto room_code = RequiredString(message.payload, "room_code");
            auto left_room_result = room_service_.LeaveRoom(room_code, participant_id);

            if (left_room_result.IsOk()) {
                auto left_room = left_room_result.Value();

                send(SuccessResponse(response_type, message.request_id, {{"room_code", room_code}}));
                event_handler_(
                    BuildControlLeftEventData(room_code, participant_id, left_room.remaining_participants),
                    left_room.remaining_participants
                );
            } else {
                std::string code = RoomServiceErrorToCode(left_room_result.Error().type);

                send(ErrorResponse(
                        response_type,
                        message.request_id,
                        code
                    )
                );

                return;
            }
            
            return;
        }

        if (message.type == ControlMessageType::Offer || message.type == ControlMessageType::Answer || message.type == ControlMessageType::IceCandidate) {
            auto target_participant_id = RequiredString(message.payload, "target_participant_id");
            auto room_code = RequiredString(message.payload, "room_code");

            // * Log
            if (!room_service_.ParticipantInRoom(participant_id, room_code)) {
                send(ErrorResponse(response_type, message.request_id, "not_in_room"));
                return;
            }
            if (!room_service_.ParticipantInRoom(target_participant_id, room_code)) {
                send(ErrorResponse(response_type, message.request_id, "not_in_room"));
                return;
            }

            ControlRelay relay_message{
                response_type,
                target_participant_id,
                participant_id,
                room_code,
                message.payload
            };

            bool success_relay = relay(relay_message);
            if (!success_relay) {
                send(ErrorResponse(response_type, message.request_id, "participant_unavailable"));
                return;
            }

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

void ControlDispatcher::HandleParticipantDisconnected(const domain::ParticipantId& participant_id) {
    auto left_room_results = room_service_.LeaveAllRooms(participant_id);

    for (const auto& left_room_result : left_room_results) {
        if (!left_room_result.IsOk()) {
            continue;
        }
        auto left_room = left_room_result.Value();
        domain::RoomCode room_code = left_room.room_code;
        event_handler_(
            BuildControlLeftEventData(room_code, participant_id, left_room.remaining_participants),
            left_room.remaining_participants
        );
    }
}

} // namespace buzzweb::app
