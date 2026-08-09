#include "buzzweb/app/ControlDispatcher.hpp"

#include <boost/log/trivial.hpp>

#include <exception>
#include <utility>

namespace buzzweb::app {
namespace {

ErrorResponse::ErrorResponsePayload MapServiceError(const RoomServiceError& error)
{
    switch (error.type) {
    case RoomServiceError::WrongPassword:
        return WrongPasswordResponse{};
    case RoomServiceError::RoomNotFound:
        return RoomNotFoundResponse{};
    case RoomServiceError::RoomCodeGenerationFailed:
        return RoomCodeGenerationFailedResponse{};
    case RoomServiceError::ParticipantUnavailable:
        return ParticipantUnavailableResponse{};
    case RoomServiceError::NotInRoom:
        return NotInRoomResponse{};
    case RoomServiceError::AlreadyJoined:
        return AlreadyInRoomResponse{};
    case RoomServiceError::RoomAlreadyExists:
        return RoomAlreadyExistsResponse{};
    case RoomServiceError::PasswordNotProvided:
        return PasswordNotProvidedResponse{};
    case RoomServiceError::InternalError:
        return InternalErrorResponse{};
    }

    return UnexpectedErrorResponse{};
}

} // namespace

ControlDispatcher::ControlDispatcher(RoomService& room_service)
    : room_service_(room_service)
{
}

ControlDispatcher::~ControlDispatcher() = default;

void ControlDispatcher::Dispatch(const ControlDispatcherRequest& request, Handler handler)
{
    if (!handler) {
        return;
    }

    const auto send_error = [&](ErrorResponse::ErrorResponsePayload payload) {
        handler({ErrorResponse{request.meta, std::move(payload)}, {}});
    };

    try {
        switch (request.meta.type) {
        case ControlMessageType::CreateRoom: {
            const auto& command = std::get<CreateRoomCommand>(request.command);
            auto create_result = room_service_.CreateRoom(command.password_hash);
            if (!create_result.IsOk()) {
                send_error(MapServiceError(create_result.Error()));
                return;
            }

            auto join_result = room_service_.JoinRoom(
                create_result.Value().GetCode(),
                command.participant,
                command.password_hash
            );
            if (!join_result.IsOk()) {
                send_error(MapServiceError(join_result.Error()));
                return;
            }

            const auto& joined = join_result.Value();
            handler({SuccessResponse{
                request.meta,
                CreateRoomResponse{joined.room_code, joined.participant, joined.room_participants}
            }, {}});
            return;
        }
        case ControlMessageType::JoinRoom: {
            const auto& command = std::get<JoinRoomCommand>(request.command);
            auto result = room_service_.JoinRoom(
                command.room_code,
                command.participant,
                command.password_hash
            );
            if (!result.IsOk()) {
                send_error(MapServiceError(result.Error()));
                return;
            }

            const auto& joined = result.Value();
            handler({
                SuccessResponse{
                    request.meta,
                    JoinRoomResponse{joined.room_code, joined.participant, joined.room_participants}
                },
                {JoinedRoomEvent{joined.room_code, joined.participant, joined.room_participants}}
            });
            return;
        }
        case ControlMessageType::LeaveRoom: {
            const auto& command = std::get<LeaveRoomCommand>(request.command);
            auto result = room_service_.LeaveRoom(command.room_code, command.participant_id);
            if (!result.IsOk()) {
                send_error(MapServiceError(result.Error()));
                return;
            }

            const auto& left = result.Value();
            handler({
                SuccessResponse{request.meta, LeaveRoomResponse{left.room_code}},
                {LeftRoomEvent{left.room_code, left.participant_id, left.remaining_participants}}
            });
            return;
        }
        case ControlMessageType::Signaling: {
            const auto& command = std::get<SignalingCommand>(request.command);
            if (!room_service_.ParticipantInRoom(command.from_participant_id, command.room_code) ||
                !room_service_.ParticipantInRoom(command.target_participant_id, command.room_code)) {
                send_error(NotInRoomResponse{});
                return;
            }

            handler({SuccessResponse{
                request.meta,
                SignalingResponse{
                    command.type,
                    command.room_code,
                    command.from_participant_id,
                    command.target_participant_id,
                    command.data
                }
            }, {}});
            return;
        }
        }
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Control dispatcher error: " << ex.what();
        send_error(InternalErrorResponse{});
    }
}

std::vector<ControlDispatcherEvent> ControlDispatcher::HandleParticipantDisconnected(
    const domain::ParticipantId& participant_id
)
{
    std::vector<ControlDispatcherEvent> events;
    for (const auto& result : room_service_.LeaveAllRooms(participant_id)) {
        if (!result.IsOk()) {
            BOOST_LOG_TRIVIAL(error) << "Disconnect cleanup failed for participant " << participant_id;
            continue;
        }

        const auto& left = result.Value();
        events.emplace_back(LeftRoomEvent{
            left.room_code,
            left.participant_id,
            left.remaining_participants
        });
    }
    return events;
}

} // namespace buzzweb::app
