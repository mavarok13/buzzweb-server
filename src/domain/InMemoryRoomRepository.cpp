#include "buzzweb/domain/InMemoryRoomRepository.hpp"

#include <utility>

namespace buzzweb::domain {
namespace {

RoomRepositoryResult Success(std::optional<RoomCode> room_code = std::nullopt)
{
    return {RoomRepositoryResult::Type::Success, {}, std::move(room_code)};
}

RoomRepositoryResult Failed(std::string message, std::optional<RoomCode> room_code = std::nullopt)
{
    return {RoomRepositoryResult::Type::Failed, std::move(message), std::move(room_code)};
}

} // namespace

InMemoryRoomRepository::InMemoryRoomRepository() = default;

InMemoryRoomRepository::~InMemoryRoomRepository() = default;

RoomRepositoryResult InMemoryRoomRepository::Add(Room room)
{
    std::lock_guard lock(mutex_);

    auto code = room.GetCode();
    auto [_, inserted] = rooms_.emplace(code, std::move(room));
    if (!inserted) {
        return Failed("room_already_exists", std::move(code));
    }

    return Success(std::move(code));
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
    std::function<RoomRepositoryResult(Room&)> updater
)
{
    std::lock_guard lock(mutex_);

    auto room = rooms_.find(code);
    if (room == rooms_.end()) {
        return Failed("room_not_found", code);
    }

    auto updated_room = room->second;
    auto result = updater(updated_room);
    if (result.type != RoomRepositoryResult::Type::Success) {
        return result;
    }

    room->second = std::move(updated_room);
    if (!result.room_code.has_value()) {
        result.room_code = code;
    }

    return result;
}

RoomRepositoryResult InMemoryRoomRepository::Remove(const RoomCode& code)
{
    std::lock_guard lock(mutex_);

    if (rooms_.erase(code) == 0) {
        return Failed("room_not_found", code);
    }

    return Success(code);
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
