#include "buzzweb/app/RoomService.hpp"

#include <boost/log/trivial.hpp>

#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace buzzweb::app {
namespace {

RoomServiceError BuildRoomAlreadyExistsError() {
    return {RoomServiceError::RoomAlreadyExists, "Room already exists"};
}
RoomServiceError BuildRoomNotFoundError() {
    return {RoomServiceError::RoomNotFound, "Room not found"};
}
RoomServiceError BuildAlreadyJoinedError() {
    return {RoomServiceError::AlreadyJoined, "Participant already in room"};
}
RoomServiceError BuildNotInRoomError() {
    return {RoomServiceError::NotInRoom, "Participant not in the room"};
}
RoomServiceError BuildRoomCodeGenerationFailedError() {
    return {RoomServiceError::RoomCodeGenerationFailed, "Room code generation failed"};
}
RoomServiceError BuildWrongPasswordError() {
    return {RoomServiceError::WrongPassword, "Wrong password"};
}
RoomServiceError BuildPasswordNotProvidedError() {
    return {RoomServiceError::PasswordNotProvided, "Password not provided"};
}
RoomServiceError BuildInternalError() {
    return {RoomServiceError::InternalError, "Internal error"};
}

domain::RoomCode GenerateRoomCode()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 999999);

    std::ostringstream stream;
    stream << std::setw(6) << std::setfill('0') << distribution(generator);
    return stream.str();
}

} // namespace

RoomService::RoomService(domain::RoomRepositoryPtr repository)
    : repository_(std::move(repository))
{
    if (!repository_) {
        throw std::invalid_argument("repository_required");
    }
}

RoomService::~RoomService() = default;

CreateRoomResult RoomService::CreateRoom(std::optional<domain::RoomSecret> password)
{
    constexpr int max_attempts = 100;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        domain::Room room(GenerateRoomCode(), password);

        try {
            auto repository_result = repository_->Add(room);
            if (repository_result == domain::AddRoomResult::Added) {
                return CreateRoomResult::Ok(room);
            }
        } catch (const std::exception& ex) {
            BOOST_LOG_TRIVIAL(error) << "Creating room failed: " << ex.what();
            return CreateRoomResult::Err(BuildInternalError());
        }
    }

    return CreateRoomResult::Err(BuildRoomCodeGenerationFailedError());
}

ParticipantJoinedRoomResult RoomService::JoinRoom(
    const domain::RoomCode& room_code,
    domain::Participant participant,
    std::optional<domain::RoomSecret> password_hash
)
{
    std::optional<domain::JoinRoomResult> join_result;
    std::vector<domain::Participant> room_participants;

    try {
        auto repository_result = repository_->Update(
            room_code,
            [&](domain::Room& room) {
                join_result = room.TryAddParticipant(participant, password_hash);

                if (*join_result != domain::JoinRoomResult::Joined) {
                    return domain::TransactionResult::Abort;
                }

                room_participants = room.GetParticipants();
                return domain::TransactionResult::Commit;
            }
        );

        if (repository_result == domain::UpdateRoomResult::NotFound) {
            return ParticipantJoinedRoomResult::Err(BuildRoomNotFoundError());
        }
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Joining room(" << room_code << ") failed: " << ex.what();
        return ParticipantJoinedRoomResult::Err(BuildInternalError());
    }

    if (!join_result) {
        BOOST_LOG_TRIVIAL(error) << "Joining room(" << room_code << ") failed: join_result has no value";
        return ParticipantJoinedRoomResult::Err(BuildInternalError());
    }
    if (*join_result == domain::JoinRoomResult::AlreadyInRoom) {
        return ParticipantJoinedRoomResult::Err(BuildAlreadyJoinedError());
    }
    if (*join_result == domain::JoinRoomResult::InvalidSecret) {
        return ParticipantJoinedRoomResult::Err(BuildWrongPasswordError());
    }
    if (*join_result == domain::JoinRoomResult::SecretNotProvided) {
        return ParticipantJoinedRoomResult::Err(BuildPasswordNotProvidedError());
    }

    ParticipantJoinedRoom joined_room(room_code, participant, room_participants);
    return ParticipantJoinedRoomResult::Ok(joined_room);
}

ParticipantLeftRoomResult RoomService::LeaveRoom(const domain::RoomCode& room_code, const domain::ParticipantId& participant_id)
{
    std::optional<domain::LeaveRoomResult> leave_result;
    std::vector<domain::Participant> remaining_participants;

    try {
        auto repository_result = repository_->Update(
            room_code,
            [&](domain::Room& room) {
                leave_result = room.RemoveParticipant(participant_id);

                if (*leave_result != domain::LeaveRoomResult::Left) {
                    return domain::TransactionResult::Abort;
                }

                remaining_participants = room.GetParticipants();
                return domain::TransactionResult::Commit;
            }
        );

        if (repository_result == domain::UpdateRoomResult::NotFound) {
            return ParticipantLeftRoomResult::Err(BuildRoomNotFoundError());
        }
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Leaving room(" << room_code << ") failed: " << ex.what();
    }
    
    if (!leave_result) {
        BOOST_LOG_TRIVIAL(error) << "Leaving room(" << room_code << ") failed: leave_result has no value";
        return ParticipantLeftRoomResult::Err(BuildInternalError());
    }
    if (leave_result == domain::LeaveRoomResult::NotInRoom) {
        return ParticipantLeftRoomResult::Err(BuildNotInRoomError());
    }
    
    try {
        auto repository_result = repository_->RemoveIfEmpty(room_code);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Leaving room(" << room_code << ") failed: " << ex.what();
        return ParticipantLeftRoomResult::Err(BuildInternalError());
    }

    return ParticipantLeftRoomResult::Ok({room_code, participant_id, remaining_participants});
}

ParticipantsLeftRoomResults RoomService::LeaveAllRooms(const domain::ParticipantId& participant_id) {
    ParticipantsLeftRoomResults left_room_results;

    for (const auto& room_code : repository_->GetRoomCodes()) {
        auto left_room_result = LeaveRoom(room_code, participant_id);
        if (left_room_result.IsOk() || left_room_result.Error().type == RoomServiceError::InternalError) {
            left_room_results.push_back(left_room_result);
        }
    }

    return left_room_results;
}

GetParticipantsResult RoomService::GetRoomParticipants(const domain::RoomCode& code) const
{
    auto room = repository_->FindByCode(code);
    if (!room) {
        return GetParticipantsResult::Err(BuildRoomNotFoundError());
    }

    return GetParticipantsResult::Ok(room->GetParticipants());
}

bool RoomService::ParticipantInRoom(
    const domain::ParticipantId& participant_id,
    const domain::RoomCode& room_code
) const
{
    return repository_->ParticipantInRoom(participant_id, room_code);
}

} // namespace buzzweb::app
