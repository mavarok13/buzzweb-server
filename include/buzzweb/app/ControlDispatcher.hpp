#pragma once

#include "buzzweb/app/RoomService.hpp"
#include "buzzweb/domain/Participant.hpp"
#include "buzzweb/domain/Room.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace buzzweb::app {

enum ControlMessageType {
    CreateRoom,
    JoinRoom,
    LeaveRoom,
    Signaling
};

enum SignalingType {
    Offer,
    Answer,
    IceCandidate
};

struct CreateRoomCommand {
    domain::Participant participant;
    std::optional<domain::RoomSecret> password_hash;
};
struct JoinRoomCommand {
    domain::RoomCode room_code;
    domain::Participant participant;
    std::optional<domain::RoomSecret> password_hash;
};
struct LeaveRoomCommand {
    domain::RoomCode room_code;
    domain::ParticipantId participant_id;
};
struct SignalingCommand {
    SignalingType type;
    domain::RoomCode room_code;
    domain::ParticipantId from_participant_id;
    domain::ParticipantId target_participant_id;
    nlohmann::json data;
};

struct RequestMeta {
    ControlMessageType type;
    std::optional<std::string> request_id;
    std::optional<SignalingType> signaling_type = std::nullopt;
};
struct ControlDispatcherRequest {
    using CommandType = std::variant<
        CreateRoomCommand,
        JoinRoomCommand,
        LeaveRoomCommand,
        SignalingCommand
    >;

    RequestMeta meta;
    CommandType command;
};

struct CreateRoomResponse {
    domain::RoomCode room_code;
    domain::Participant participant;
    std::vector<domain::Participant> participants;
};
struct JoinRoomResponse {
    domain::RoomCode room_code;
    domain::Participant participant;
    std::vector<domain::Participant> participants;
};
struct LeaveRoomResponse {
    domain::RoomCode room_code;
};
struct SignalingResponse {
    SignalingType type;
    domain::RoomCode room_code;
    domain::ParticipantId from_participant_id;
    domain::ParticipantId target_participant_id;
    nlohmann::json data;
};

struct RoomAlreadyExistsResponse {};
struct RoomCodeGenerationFailedResponse {};
struct NotInRoomResponse {};
struct AlreadyInRoomResponse {};
struct WrongPasswordResponse {};
struct PasswordNotProvidedResponse {};
struct RoomNotFoundResponse {};
struct ParticipantUnavailableResponse {};
struct InternalErrorResponse {};
struct UnexpectedErrorResponse {};

using ResponseMeta = RequestMeta;
struct SuccessResponse {
    using SuccessResponsePayload = std::variant<
        CreateRoomResponse,
        JoinRoomResponse,
        LeaveRoomResponse,
        SignalingResponse
    >;

    ResponseMeta meta;
    SuccessResponsePayload payload;
};
struct ErrorResponse {
    using ErrorResponsePayload = std::variant<
        RoomAlreadyExistsResponse,
        RoomCodeGenerationFailedResponse,
        NotInRoomResponse,
        AlreadyInRoomResponse,
        WrongPasswordResponse,
        PasswordNotProvidedResponse,
        RoomNotFoundResponse,
        ParticipantUnavailableResponse,
        InternalErrorResponse,
        UnexpectedErrorResponse
    >;

    ResponseMeta meta;
    ErrorResponsePayload payload;
};

using ControlDispatcherResponse = std::variant<SuccessResponse, ErrorResponse>;

struct JoinedRoomEvent {
    domain::RoomCode room_code;
    domain::Participant joined_participant;
    std::vector<domain::Participant> event_receivers;
};
struct LeftRoomEvent {
    domain::RoomCode room_code;
    domain::ParticipantId left_participant_id;
    std::vector<domain::Participant> event_receivers;
};

using ControlDispatcherEvent = std::variant<JoinedRoomEvent, LeftRoomEvent>;

struct ControlDispatcherResult {
    ControlDispatcherResponse response;
    std::vector<ControlDispatcherEvent> events;
};


class ControlDispatcher {
public:
    using Handler = std::function<void(const ControlDispatcherResult& response)>;

    explicit ControlDispatcher(RoomService& room_service);
    ~ControlDispatcher();

    void Dispatch(const ControlDispatcherRequest& request, Handler handler);

    std::vector<ControlDispatcherEvent> HandleParticipantDisconnected(
        const domain::ParticipantId& participant_id
    );

private:
    RoomService& room_service_;
};

} // namespace buzzweb::app
