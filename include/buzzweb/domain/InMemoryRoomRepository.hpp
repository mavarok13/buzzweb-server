#pragma once

#include "buzzweb/domain/RoomRepository.hpp"

#include <mutex>
#include <unordered_map>

namespace buzzweb::domain {

class InMemoryRoomRepository final : public RoomRepository {
public:
    InMemoryRoomRepository();
    ~InMemoryRoomRepository() override;

    AddRoomResult Add(Room room) override;
    std::optional<Room> FindByCode(const RoomCode& room_code) const override;
    UpdateRoomResult Update(const RoomCode& room_code, std::function<TransactionResult(Room&)> updater) override;
    RemoveRoomResult Remove(const RoomCode& room_code) override;
    RemoveEmptyRoomResult RemoveIfEmpty(const RoomCode& room_code) override;
    bool Exists(const RoomCode& room_code) const override;
    std::vector<RoomCode> GetRoomCodes() const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<RoomCode, Room> rooms_;
};

} // namespace buzzweb::domain
