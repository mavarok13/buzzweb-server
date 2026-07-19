#pragma once

#include "buzzweb/domain/Room.hpp"

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include <string>

namespace buzzweb::domain {

enum AddRoomResult {
    Added,
    AlreadyExists
};
enum UpdateRoomResult {
    Updated,
    NotFound,
    Aborted
};
enum RemoveRoomResult {
    Removed,
    NotFound
};
enum RemoveEmptyRoomResult {
    Removed,
    NotEmpty,
    NotFound
};

enum TransactionResult {
    Commit,
    Abort
};

class RoomRepository {
public:
    virtual ~RoomRepository() = default;

    virtual AddRoomResult Add(Room room) = 0;
    virtual std::optional<Room> FindByCode(const RoomCode& room_code) const = 0;
    virtual UpdateRoomResult Update(const RoomCode& room_code, std::function<TransactionResult(Room&)> updater) = 0;
    virtual RemoveRoomResult Remove(const RoomCode& room_code) = 0;
    virtual RemoveEmptyRoomResult RemoveIfEmpty(const RoomCode& room_code) = 0;
    virtual bool Exists(const RoomCode& room_code) const = 0;
    virtual std::vector<RoomCode> GetRoomCodes() const = 0;
};

using RoomRepositoryPtr = std::shared_ptr<RoomRepository>;

} // namespace buzzweb::domain
