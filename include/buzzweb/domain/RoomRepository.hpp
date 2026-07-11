#pragma once

#include "buzzweb/domain/Room.hpp"

#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include <string>

namespace buzzweb::domain {

struct RoomRepositoryResult {
public:
    enum class Type {
        Success,
        Failed
    };

    Type type;
    std::string message;
    std::optional<RoomCode> room_code;
};

enum RoomRepositoryDecision {
    Abort,
    Commit
};

class RoomRepository {
public:
    virtual ~RoomRepository() = default;

    virtual RoomRepositoryResult Add(Room room) = 0;
    virtual std::optional<Room> FindByCode(const RoomCode& code) const = 0;
    virtual RoomRepositoryResult Update(const RoomCode& code, std::function<RoomRepositoryDecision(Room&)> updater) = 0;
    virtual RoomRepositoryResult Remove(const RoomCode& code) = 0;
    virtual RoomRepositoryResult RemoveIfEmpty(const RoomCode& code) = 0;
    virtual bool Exists(const RoomCode& code) const = 0;
    virtual bool ParticipantInRoom(const ParticipantId& participant_id, const RoomCode& room_code) const = 0;
    virtual std::vector<RoomCode> GetRoomCodes() const = 0;
};

using RoomRepositoryPtr = std::shared_ptr<RoomRepository>;

} // namespace buzzweb::domain
