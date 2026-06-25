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

domain::RoomRepositoryResult Success(const domain::RoomCode& code)
{
    return {domain::RoomRepositoryResult::Type::Success, {}, code};
}

domain::RoomRepositoryResult Failed(std::string message, const domain::RoomCode& code)
{
    return {domain::RoomRepositoryResult::Type::Failed, std::move(message), code};
}

void ThrowIfFailed(const domain::RoomRepositoryResult& result)
{
    if (result.type == domain::RoomRepositoryResult::Type::Failed) {
        throw std::runtime_error(result.message.empty() ? "repository_error" : result.message);
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
            ThrowIfFailed(result);
        }
    }

    throw std::runtime_error("room_code_generation_failed");
}

domain::Room RoomService::JoinRoom(
    const domain::RoomCode& code,
    domain::Participant participant,
    std::optional<std::string> password_hash
)
{
    std::optional<domain::Room> joined_room;

    auto result = repository_->Update(
        code,
        [participant = std::move(participant), password_hash = std::move(password_hash), &joined_room](domain::Room& room) {
            if (room.IsPasswordProtected() && room.GetPasswordHash() != password_hash) {
                return Failed("wrong_password", room.GetCode());
            }

            if (room.HasParticipant(participant.GetId())) {
                return Failed("already_joined", room.GetCode());
            }

            room.AddParticipant(participant);
            joined_room = room;
            return Success(room.GetCode());
        }
    );

    ThrowIfFailed(result);
    if (!joined_room.has_value()) {
        throw std::runtime_error("internal_error");
    }

    return *joined_room;
}

void RoomService::LeaveRoom(const domain::RoomCode& code, const domain::ParticipantId& participant_id)
{
    auto result = repository_->Update(
        code,
        [&participant_id](domain::Room& room) {
            if (!room.HasParticipant(participant_id)) {
                return Failed("not_in_room", room.GetCode());
            }

            room.RemoveParticipant(participant_id);
            return Success(room.GetCode());
        }
    );

    ThrowIfFailed(result);
}

std::vector<domain::Participant> RoomService::GetRoomParticipants(const domain::RoomCode& code) const
{
    auto room = repository_->FindByCode(code);
    if (!room.has_value()) {
        throw std::runtime_error("room_not_found");
    }

    return room->GetParticipants();
}

} // namespace buzzweb::app
