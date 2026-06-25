#pragma once

#include "buzzweb/domain/RoomRepository.hpp"

#include <mutex>
#include <unordered_map>

namespace buzzweb::domain {

class InMemoryRoomRepository final : public RoomRepository {
public:
    InMemoryRoomRepository();
    ~InMemoryRoomRepository() override;

    RoomRepositoryResult Add(Room room) override;
    std::optional<Room> FindByCode(const RoomCode& code) const override;
    RoomRepositoryResult Update(const RoomCode& code, std::function<RoomRepositoryResult(Room&)> updater) override;
    RoomRepositoryResult Remove(const RoomCode& code) override;
    bool Exists(const RoomCode& code) const override;
    std::vector<RoomCode> GetRoomCodes() const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<RoomCode, Room> rooms_;
};

} // namespace buzzweb::domain
