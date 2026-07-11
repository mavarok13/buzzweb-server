#include "buzzweb/domain/InMemoryRoomRepository.hpp"

#include <utility>

namespace buzzweb::domain {

InMemoryRoomRepository::InMemoryRoomRepository() = default;

InMemoryRoomRepository::~InMemoryRoomRepository() = default;

RoomRepositoryResult BuildSuccessRoomRepositoryResult(RoomCode code)
{
    return RoomRepositoryResult{
        .type = RoomRepositoryResult::Type::Success,
        .message = "success",
        .room_code = std::move(code)
    };
}

RoomRepositoryResult BuildFailedRoomRepositoryResult(std::string message, RoomCode code)
{
    return RoomRepositoryResult{
        .type = RoomRepositoryResult::Type::Failed,
        .message = std::move(message),
        .room_code = std::move(code)
    };
}

RoomRepositoryResult InMemoryRoomRepository::Add(Room room)
{
    std::lock_guard lock(mutex_);

    auto code = room.GetCode();
    auto [_, inserted] = rooms_.emplace(code, std::move(room));
    if (!inserted) {
        return BuildFailedRoomRepositoryResult("room_already_exists", std::move(code));
    }

    return BuildSuccessRoomRepositoryResult(std::move(code));
}

std::optional<Room> InMemoryRoomRepository::FindByCode(const RoomCode& code) const
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(code);
    if (room == rooms_.end()) {
        return std::nullopt;
    }

    return room->second;
}

RoomRepositoryResult InMemoryRoomRepository::Update(
    const RoomCode& code,
    std::function<RoomRepositoryDecision(Room&)> updater
)
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(code);
    if (room == rooms_.end()) {
        return BuildFailedRoomRepositoryResult("room_not_found", code);
    }

    auto updated_room = room->second;
    auto result = updater(updated_room);
    if (result == RoomRepositoryDecision::Abort) {
        return BuildFailedRoomRepositoryResult("update_aborted", code);
    }

    room->second = std::move(updated_room);
    if (result == RoomRepositoryDecision::Commit) {
        return BuildSuccessRoomRepositoryResult(code);
    }

    return BuildFailedRoomRepositoryResult("update_failed", code);
}

RoomRepositoryResult InMemoryRoomRepository::Remove(const RoomCode& code)
{
    std::lock_guard lock(mutex_);

    if (rooms_.erase(code) == 0) {
        return BuildFailedRoomRepositoryResult("room_not_found", code);
    }

    return BuildSuccessRoomRepositoryResult(code);
}

RoomRepositoryResult InMemoryRoomRepository::RemoveIfEmpty(const RoomCode& code)
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(code);
    if (room == rooms_.end()) {
        return BuildFailedRoomRepositoryResult("room_not_found", code);
    }

    if (room->second.IsEmpty()) {
        rooms_.erase(room);
        return BuildSuccessRoomRepositoryResult(code);
    }

    return BuildSuccessRoomRepositoryResult(code);
}

bool InMemoryRoomRepository::Exists(const RoomCode& code) const
{
    std::lock_guard lock(mutex_);
    return rooms_.contains(code);
}

std::vector<RoomCode> InMemoryRoomRepository::GetRoomCodes() const
{
    std::lock_guard lock(mutex_);

    std::vector<RoomCode> codes;
    codes.reserve(rooms_.size());
    for (const auto& [code, _] : rooms_) {
        codes.push_back(code);
    }

    return codes;
}

} // namespace buzzweb::domain
