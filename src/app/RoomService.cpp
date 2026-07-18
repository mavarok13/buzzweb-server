#include "buzzweb/app/RoomService.hpp"

#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace buzzweb::app {
namespace {

Error BuildRoomAlreadyExistsError() {
    return {ErrorType::RoomAlreadyExists, "Room already exists"};
}

Error BuildRoomCodeGenerationFailedError() {
    return {ErrorType::RoomCodeGenerationFailed, "Room code generation failed"};
}

Error BuildWrongPasswordError() {
    return {ErrorType::WrongPassword, "Wrong password"};
}

Error BuildAlreadyJoinedError() {
    return {ErrorType::AlreadyJoined, "Participant already in room"};
}

Error BuildRoomNotFoundError() {
    return {ErrorType::RoomNotFound, "Room not found"};
}

Error BuildInternalError() {
    return {ErrorType::InternalError, "Internal error"};
}

domain::RoomCode GenerateRoomCode()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 999999);

    std::ostringstream stream;
    stream << std::setw(6) << std::setfill('0') << distribution(generator);
    return stream.str();
}

// void ThrowIfFailed(const std::optional<std::string>& error)
// {
//     if (error.has_value()) {
//         throw std::runtime_error(error.value());
//     }
// }

} // namespace

RoomService::RoomService(domain::RoomRepositoryPtr repository)
    : repository_(std::move(repository))
{
    if (!repository_) {
        throw std::invalid_argument("repository_required");
    }
}

RoomService::~RoomService() = default;

CreateRoomResult RoomService::CreateRoom(std::optional<std::string> password)
{
    constexpr int max_attempts = 100;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        domain::Room room(GenerateRoomCode(), password);
        auto result = repository_->Add(room);
        if (result.type == domain::RoomRepositoryResult::Type::Success) {
            return CreateRoomResult::Ok(room);
        } else {
            if (result.result == domain::RoomRepositoryResult::Result::RoomAlreadyExists) {
                continue;
            } else {
                return CreateRoomResult::Err(BuildInternalError());
            }
        }
    }

    return CreateRoomResult::Err(BuildRoomCodeGenerationFailedError());
}

ParticipantJoinedRoomResult RoomService::JoinRoom(
    const domain::RoomCode& code,
    domain::Participant participant,
    std::optional<std::string> password_hash
)
{
    std::optional<domain::Room> room;
    std::optional<Error> error;

    std::vector<domain::Participant> room_participants;

    auto repository_result = repository_->Update(
        code,
        [code, participant, password_hash = std::move(password_hash), joined_room = &room, &error, &room_participants](domain::Room& room) {
            if (room.IsPasswordProtected() && room.GetPasswordHash() != password_hash) {
                error = std::make_optional<Error>(ErrorType::WrongPassword, "Wrong password");
                return domain::RoomRepositoryDecision::Abort;
            }

            if (room.HasParticipant(participant.GetId())) {
                error = std::make_optional<Error>(ErrorType::AlreadyJoined, "Participant already in the room");
                return domain::RoomRepositoryDecision::Abort;
            }

            room.AddParticipant(participant);
            *joined_room = room;
            room_participants = room.GetParticipants();
            return domain::RoomRepositoryDecision::Commit;
        }
    );

    if (error) {
        return ParticipantJoinedRoomResult::Err(*error);
    }
    if (repository_result.type == domain::RoomRepositoryResult::Type::Failed) {
        if (repository_result.result == domain::RoomRepositoryResult::Result::RoomNotFound) {
            error = std::make_optional<Error>(ErrorType::RoomNotFound, repository_result.message);
            return ParticipantJoinedRoomResult::Err(*error);
        }

        error = std::make_optional<Error>(ErrorType::InternalError, repository_result.message);
        return ParticipantJoinedRoomResult::Err(*error);
    }
    if (!room.has_value()) {
        error = std::make_optional<Error>(ErrorType::UnknownError, "joined_room has no value");
        return ParticipantJoinedRoomResult::Err(*error);
    }

    ParticipantJoinedRoom joined_room(code, participant, room_participants);
    return ParticipantJoinedRoomResult::Ok(joined_room);
}

ParticipantLeftRoomResult RoomService::LeaveRoom(const domain::RoomCode& code, const domain::ParticipantId& participant_id)
{
    std::vector<domain::Participant> remaining_participants;

    std::optional<Error> error;
    auto repository_result = repository_->Update(
        code,
        [&participant_id, &error, &remaining_participants](domain::Room& room) {
            if (!room.HasParticipant(participant_id)) {
                error = std::make_optional<Error>(ErrorType::NotInRoom, "Participant not in room");
                return domain::RoomRepositoryDecision::Abort;
            }

            room.RemoveParticipant(participant_id);
            remaining_participants = room.GetParticipants();
            return domain::RoomRepositoryDecision::Commit;
        }
    );

    if (error) {
        return ParticipantLeftRoomResult::Err(*error);
    } 
    if (repository_result.type == domain::RoomRepositoryResult::Type::Failed) {
        if (repository_result.result == domain::RoomRepositoryResult::Result::RoomNotFound) {
            error = std::make_optional<Error>(ErrorType::RoomNotFound, repository_result.message);
            return ParticipantLeftRoomResult::Err(*error);
        }

        error = std::make_optional<Error>(ErrorType::InternalError, repository_result.message);
        return ParticipantLeftRoomResult::Err(*error);
    }
    
    auto remove_room_repos_result = repository_->RemoveIfEmpty(code);
    if (remove_room_repos_result.type == domain::RoomRepositoryResult::Type::Failed) {
        if (remove_room_repos_result.result == domain::RoomRepositoryResult::Result::RoomNotFound) {
            error = std::make_optional<Error>(ErrorType::RoomNotFound, remove_room_repos_result.message);
            return ParticipantLeftRoomResult::Err(*error);
        }

        error = std::make_optional<Error>(ErrorType::InternalError, remove_room_repos_result.message);
        return ParticipantLeftRoomResult::Err(*error);
    }

    ParticipantLeftRoom left_room(code, participant_id, remaining_participants);
    return ParticipantLeftRoomResult::Ok(left_room);
}

ParticipantsLeftRoomResults RoomService::LeaveAllRooms(const domain::ParticipantId& participant_id) {
    ParticipantsLeftRoomResults left_room_results;

    for (const auto& room_code : repository_->GetRoomCodes()) {
        try {
            if (!repository_->ParticipantInRoom(participant_id, room_code)) {
                continue;
            }

            left_room_results.push_back(LeaveRoom(room_code, participant_id));
        } catch (const std::exception& ex) {
            left_room_results.push_back(ParticipantLeftRoomResult::Err(Error(ErrorType::InternalError, ex.what())));
        }
    }

    return left_room_results;
}

GetParticipantsResult RoomService::GetRoomParticipants(const domain::RoomCode& code) const
{
    auto room = repository_->FindByCode(code);
    if (!room.has_value()) {
        return GetParticipantsResult::Err(BuildRoomNotFoundError());
    }

    return GetParticipantsResult::Ok(room->GetParticipants());
}

bool RoomService::ParticipantInRoom(const domain::ParticipantId& participant_id, const domain::RoomCode& room_code) const {
    return repository_->ParticipantInRoom(participant_id, room_code);
}

} // namespace buzzweb::app
