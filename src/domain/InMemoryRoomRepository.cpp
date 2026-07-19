#include "buzzweb/domain/InMemoryRoomRepository.hpp"

#include <utility>

namespace buzzweb::domain {

InMemoryRoomRepository::InMemoryRoomRepository() = default;

InMemoryRoomRepository::~InMemoryRoomRepository() = default;

AddRoomResult InMemoryRoomRepository::Add(Room room)
{
    std::lock_guard lock(mutex_);

    auto room_code = room.GetCode();
    auto [_, inserted] = rooms_.emplace(room_code, std::move(room));
    if (!inserted) {
        return AddRoomResult::AlreadyExists;
    }

    return AddRoomResult::Added;
}

std::optional<Room> InMemoryRoomRepository::FindByCode(const RoomCode& room_code) const
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(room_code);
    if (room == rooms_.end()) {
        return std::nullopt;
    }

    return room->second;
}

UpdateRoomResult InMemoryRoomRepository::Update(
    const RoomCode& room_code,
    std::function<TransactionResult(Room&)> updater
)
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(room_code);
    if (room == rooms_.end()) {
        return UpdateRoomResult::NotFound;
    }

    auto updated_room = room->second;
    auto result = updater(updated_room);
    if (result == TransactionResult::Abort) {
        return UpdateRoomResult::Aborted;
    }

    room->second = std::move(updated_room);
    return UpdateRoomResult::Updated;
}

RemoveRoomResult InMemoryRoomRepository::Remove(const RoomCode& room_code)
{
    std::lock_guard lock(mutex_);

    if (rooms_.erase(room_code) == 0) {
        return RemoveRoomResult::NotFound;
    }

    return RemoveRoomResult::Removed;
}

RemoveEmptyRoomResult InMemoryRoomRepository::RemoveIfEmpty(const RoomCode& room_code)
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(room_code);
    if (room == rooms_.end()) {
        return RemoveEmptyRoomResult::NotFound;
    }

    if (room->second.IsEmpty()) {
        rooms_.erase(room);
        return RemoveEmptyRoomResult::Removed;
    }

    return RemoveEmptyRoomResult::NotEmpty;
}

bool InMemoryRoomRepository::Exists(const RoomCode& room_code) const
{
    std::lock_guard lock(mutex_);
    return rooms_.contains(room_code);
}

std::vector<RoomCode> InMemoryRoomRepository::GetRoomCodes() const
{
    std::lock_guard lock(mutex_);

    std::vector<RoomCode> codes;
    codes.reserve(rooms_.size());
    for (const auto& [room_code, _] : rooms_) {
        codes.push_back(room_code);
    }

    return codes;
}

} // namespace buzzweb::domain
