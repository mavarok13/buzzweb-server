#include "buzzweb/app/RoomService.hpp"

#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace buzzweb::app {
namespace {

domain::RoomCode GenerateRoomCode()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 999999);

    std::ostringstream stream;
    stream << std::setw(6) << std::setfill('0') << distribution(generator);
    return stream.str();
}

void ThrowIfFailed(const std::optional<std::string>& error)
{
    if (error.has_value()) {
        throw std::runtime_error(error.value());
    }
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

domain::Room RoomService::CreateRoom(std::optional<std::string> password)
{
    constexpr int max_attempts = 100;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        domain::Room room(GenerateRoomCode(), password);
        auto result = repository_->Add(room);
        if (result.type == domain::RoomRepositoryResult::Type::Success) {
            return room;
        }

        if (result.message != "room_already_exists") {
            ThrowIfFailed(result.message);
        }
    }

    throw std::runtime_error("room_code_generation_failed");
}

RoomService::ParticipantJoinedRoom RoomService::JoinRoom(
    const domain::RoomCode& code,
    domain::Participant participant,
    std::optional<std::string> password_hash
)
{
    std::optional<domain::Room> joined_room;
    std::optional<std::string> service_error;

    std::vector<domain::Participant> room_participants;

    auto repository_result = repository_->Update(
        code,
        [participant, password_hash = std::move(password_hash), &joined_room, &service_error, &room_participants](domain::Room& room) {
            if (room.IsPasswordProtected() && room.GetPasswordHash() != password_hash) {
                service_error = "wrong_password";
                return domain::RoomRepositoryDecision::Abort;
            }

            if (room.HasParticipant(participant.GetId())) {
                service_error = "already_joined";
                return domain::RoomRepositoryDecision::Abort;
            }

            room.AddParticipant(participant);
            joined_room = room;
            room_participants = room.GetParticipants();
            return domain::RoomRepositoryDecision::Commit;
        }
    );

    ThrowIfFailed(service_error);
    if (repository_result .type == domain::RoomRepositoryResult::Type::Failed) {
        ThrowIfFailed(repository_result.message);
    }
    if (!joined_room.has_value()) {
        throw std::runtime_error("internal_error");
    }

    return {code, participant, room_participants};
}

RoomService::ParticipantLeftRoom RoomService::LeaveRoom(const domain::RoomCode& code, const domain::ParticipantId& participant_id)
{
    std::vector<domain::Participant> remaining_participants;

    std::optional<std::string> service_error;
    auto repository_result = repository_->Update(
        code,
        [&participant_id, &service_error, &remaining_participants](domain::Room& room) {
            if (!room.HasParticipant(participant_id)) {
                service_error = "not_in_room";
                return domain::RoomRepositoryDecision::Abort;
            }

            room.RemoveParticipant(participant_id);
            remaining_participants = room.GetParticipants();
            return domain::RoomRepositoryDecision::Commit;
        }
    );

    ThrowIfFailed(service_error);
    if (repository_result.type == domain::RoomRepositoryResult::Type::Failed) {
        ThrowIfFailed(repository_result.message);
    }

    auto remove_room_repos_result = repository_->RemoveIfEmpty(code);
    if (remove_room_repos_result.type == domain::RoomRepositoryResult::Type::Failed) {
        ThrowIfFailed(remove_room_repos_result.message);
    }

    return {code, participant_id, remaining_participants};
}

std::vector<RoomService::ParticipantLeftRoom> RoomService::LeaveAllRooms(const domain::ParticipantId& participant_id) {
    std::vector<RoomService::ParticipantLeftRoom> left_room;

    for (const auto& room_code : repository_->GetRoomCodes()) {
        try {
            if (!repository_->ParticipantInRoom(participant_id, room_code)) {
                continue;
            }

            left_room.push_back(LeaveRoom(room_code, participant_id));
        } catch (const std::runtime_error& error) {
            const std::string code = error.what();
            if (code == "room_not_found" || code == "not_in_room") {
                continue;
            }
            throw;
        }
    }

    return left_room;
}

std::vector<domain::Participant> RoomService::GetRoomParticipants(const domain::RoomCode& code) const
{
    auto room = repository_->FindByCode(code);
    if (!room.has_value()) {
        throw std::runtime_error("room_not_found");
    }

    return room->GetParticipants();
}

bool RoomService::ParticipantInRoom(const domain::ParticipantId& participant_id, const domain::RoomCode& room_code) const {
    return repository_->ParticipantInRoom(participant_id, room_code);
}

} // namespace buzzweb::app
