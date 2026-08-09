#include "buzzweb/app/ProtocolCodec.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace buzzweb::app {
namespace {

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

nlohmann::json ParticipantToJson(const domain::Participant& participant)
{
    return {
        {"participant_id", participant.GetId()},
        {"display_name", participant.GetName()}
    };
}

nlohmann::json ParticipantsToJson(const std::vector<domain::Participant>& participants)
{
    auto json = nlohmann::json::array();
    for (const auto& participant : participants) {
        json.push_back(ParticipantToJson(participant));
    }
    return json;
}

std::string SignalingTypeToString(SignalingType type)
{
    switch (type) {
    case SignalingType::Offer:
        return "offer";
    case SignalingType::Answer:
        return "answer";
    case SignalingType::IceCandidate:
        return "ice_candidate";
    }

    throw std::invalid_argument("invalid_message");
}

std::string ResultType(const ResponseMeta& meta)
{
    switch (meta.type) {
    case ControlMessageType::CreateRoom:
        return "create_room_result";
    case ControlMessageType::JoinRoom:
        return "join_room_result";
    case ControlMessageType::LeaveRoom:
        return "leave_room_result";
    case ControlMessageType::Signaling:
        if (meta.signaling_type) {
            return SignalingTypeToString(*meta.signaling_type) + "_result";
        }
        return "error";
    }

    return "error";
}

std::string ErrorMessage(const std::string& code)
{
    if (code == "invalid_json") return "Message must be valid JSON.";
    if (code == "invalid_message") return "Message type or payload is invalid.";
    if (code == "missing_field") return "Required field is missing.";
    if (code == "invalid_field") return "Field has an invalid value.";
    if (code == "room_not_found") return "Room was not found.";
    if (code == "wrong_password") return "Room password is incorrect.";
    if (code == "already_joined") return "Participant is already in the room.";
    if (code == "not_in_room") return "Participant is not in the room.";
    if (code == "participant_unavailable") return "Participant is not connected.";
    if (code == "password_not_provided") return "Room password is required.";
    if (code == "room_code_generation_failed") return "Room code could not be generated.";
    if (code == "room_already_exists") return "Room already exists.";
    return "Internal server error.";
}

std::string ErrorCode(const ErrorResponse::ErrorResponsePayload& payload)
{
    return std::visit([](const auto& error) -> std::string {
        using Error = std::decay_t<decltype(error)>;
        if constexpr (std::is_same_v<Error, RoomAlreadyExistsResponse>) return "room_already_exists";
        if constexpr (std::is_same_v<Error, RoomCodeGenerationFailedResponse>) return "room_code_generation_failed";
        if constexpr (std::is_same_v<Error, NotInRoomResponse>) return "not_in_room";
        if constexpr (std::is_same_v<Error, AlreadyInRoomResponse>) return "already_joined";
        if constexpr (std::is_same_v<Error, WrongPasswordResponse>) return "wrong_password";
        if constexpr (std::is_same_v<Error, PasswordNotProvidedResponse>) return "password_not_provided";
        if constexpr (std::is_same_v<Error, RoomNotFoundResponse>) return "room_not_found";
        if constexpr (std::is_same_v<Error, ParticipantUnavailableResponse>) return "participant_unavailable";
        return "internal_error";
    }, payload);
}

} // namespace

ProtocolDecodeError::ProtocolDecodeError(
    std::optional<RequestMeta> meta,
    std::string code,
    std::optional<std::string> request_id
)
    : std::invalid_argument(std::move(code)),
      meta_(std::move(meta)),
      request_id_(meta_ && meta_->request_id ? meta_->request_id : std::move(request_id))
{
}

const std::optional<RequestMeta>& ProtocolDecodeError::GetMeta() const
{
    return meta_;
}

const std::optional<std::string>& ProtocolDecodeError::GetRequestId() const
{
    return request_id_;
}

ControlDispatcherRequest DecodeRequest(
    const domain::ParticipantId& participant_id,
    const std::string& message
)
{
    const auto json = nlohmann::json::parse(message);
    if (!json.is_object() || !json.contains("type") || !json.at("type").is_string()) {
        throw ProtocolDecodeError(std::nullopt, "invalid_message");
    }

    const auto type = json.at("type").get<std::string>();
    std::optional<std::string> request_id;
    if (json.contains("request_id") && json.at("request_id").is_string()) {
        request_id = json.at("request_id").get<std::string>();
    }
    std::optional<RequestMeta> meta;
    if (type == "create_room") {
        meta = RequestMeta{ControlMessageType::CreateRoom, std::nullopt};
    } else if (type == "join_room") {
        meta = RequestMeta{ControlMessageType::JoinRoom, std::nullopt};
    } else if (type == "leave_room") {
        meta = RequestMeta{ControlMessageType::LeaveRoom, std::nullopt};
    } else if (type == "offer") {
        meta = RequestMeta{ControlMessageType::Signaling, std::nullopt, SignalingType::Offer};
    } else if (type == "answer") {
        meta = RequestMeta{ControlMessageType::Signaling, std::nullopt, SignalingType::Answer};
    } else if (type == "ice_candidate") {
        meta = RequestMeta{ControlMessageType::Signaling, std::nullopt, SignalingType::IceCandidate};
    } else {
        throw ProtocolDecodeError(std::nullopt, "invalid_message", request_id);
    }

    if (json.contains("request_id") && !json.at("request_id").is_null() && !json.at("request_id").is_string()) {
        throw ProtocolDecodeError(meta, "invalid_message");
    }
    if (json.contains("request_id") && json.at("request_id").is_string()) {
        meta->request_id = request_id;
    }
    if (!json.contains("payload") || !json.at("payload").is_object()) {
        throw ProtocolDecodeError(meta, "invalid_message");
    }

    const auto& payload = json.at("payload");
    try {
        if (type == "create_room") {
            CreateRoomCommand command{
                domain::Participant(participant_id, RequiredString(payload, "display_name")),
                OptionalString(payload, "password")
            };
            return {*meta, std::move(command)};
        }
        if (type == "join_room") {
            JoinRoomCommand command{
                RequiredString(payload, "room_code"),
                domain::Participant(participant_id, RequiredString(payload, "display_name")),
                OptionalString(payload, "password")
            };
            return {*meta, std::move(command)};
        }
        if (type == "leave_room") {
            LeaveRoomCommand command{RequiredString(payload, "room_code"), participant_id};
            return {*meta, std::move(command)};
        }

        SignalingCommand command{
            *meta->signaling_type,
            RequiredString(payload, "room_code"),
            participant_id,
            RequiredString(payload, "target_participant_id"),
            payload
        };
        return {*meta, std::move(command)};
    } catch (const std::invalid_argument& ex) {
        throw ProtocolDecodeError(meta, ex.what());
    }
}

std::string EncodeResponse(const ControlDispatcherResponse& response)
{
    return std::visit([](const auto& typed_response) {
        using Response = std::decay_t<decltype(typed_response)>;
        nlohmann::json json = {
            {"type", ResultType(typed_response.meta)},
            {"ok", std::is_same_v<Response, SuccessResponse>}
        };
        if (typed_response.meta.request_id) {
            json["request_id"] = *typed_response.meta.request_id;
        }

        if constexpr (std::is_same_v<Response, ErrorResponse>) {
            const auto code = ErrorCode(typed_response.payload);
            json["error"] = {{"code", code}, {"message", ErrorMessage(code)}};
        } else {
            json["payload"] = std::visit([](const auto& payload) -> nlohmann::json {
                using Payload = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<Payload, CreateRoomResponse> ||
                              std::is_same_v<Payload, JoinRoomResponse>) {
                    return {
                        {"room_code", payload.room_code},
                        {"participant", ParticipantToJson(payload.participant)},
                        {"participants", ParticipantsToJson(payload.participants)}
                    };
                }
                return {{"room_code", payload.room_code}};
            }, typed_response.payload);
        }
        return json.dump();
    }, response);
}

std::string EncodeEvent(const ControlDispatcherEvent& event)
{
    return std::visit([](const auto& typed_event) {
        using Event = std::decay_t<decltype(typed_event)>;
        nlohmann::json payload = {
            {"room_code", typed_event.room_code},
            {"participants", ParticipantsToJson(typed_event.event_receivers)}
        };
        std::string type;
        if constexpr (std::is_same_v<Event, JoinedRoomEvent>) {
            type = "participant_joined";
            payload["participant"] = ParticipantToJson(typed_event.joined_participant);
        } else {
            type = "participant_left";
            payload["participant_id"] = typed_event.left_participant_id;
        }
        return nlohmann::json{{"type", type}, {"payload", std::move(payload)}}.dump();
    }, event);
}

std::string EncodeSignalingEvent(const SignalingResponse& response)
{
    auto payload = response.data;
    payload["room_code"] = response.room_code;
    payload["target_participant_id"] = response.target_participant_id;
    payload["from_participant_id"] = response.from_participant_id;
    return nlohmann::json{
        {"type", SignalingTypeToString(response.type)},
        {"payload", std::move(payload)}
    }.dump();
}

std::string EncodeProtocolError(
    const std::optional<RequestMeta>& meta,
    const std::string& code,
    std::optional<std::string> request_id
)
{
    nlohmann::json json = {
        {"type", meta ? ResultType(*meta) : "error"},
        {"ok", false},
        {"error", {{"code", code}, {"message", ErrorMessage(code)}}}
    };
    if (meta && meta->request_id) {
        json["request_id"] = *meta->request_id;
    } else if (request_id) {
        json["request_id"] = *request_id;
    }
    return json.dump();
}

} // namespace buzzweb::app
