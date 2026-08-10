#pragma once

#include "buzzweb/domain/Room.hpp"

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include <string>

namespace buzzweb::domain {

enum class AddRoomResult {
    Added,
    AlreadyExists
};
enum class UpdateRoomResult {
    Updated,
    NotFound,
    Aborted
};
enum class RemoveRoomResult {
    Removed,
    NotFound
};
enum class RemoveEmptyRoomResult {
    Removed,
    NotEmpty,
    NotFound
};

enum class TransactionResult {
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
    virtual bool ParticipantInRoom(
        const ParticipantId& participant_id,
        const RoomCode& room_code
    ) const = 0;
    virtual std::vector<RoomCode> GetRoomCodes() const = 0;
};

using RoomRepositoryPtr = std::shared_ptr<RoomRepository>;

} // namespace buzzweb::domain
