#pragma once

#include "buzzweb/domain/Participant.hpp"
#include "buzzweb/domain/Room.hpp"
#include "buzzweb/domain/RoomRepository.hpp"

#include <optional>
#include <string>
#include <vector>

namespace buzzweb::app {

class RoomService {
public:
    explicit RoomService(domain::RoomRepositoryPtr repository);
    ~RoomService();

    domain::Room CreateRoom(std::optional<std::string> password_hash);
    domain::Room JoinRoom(
        const domain::RoomCode& code,
        domain::Participant participant,
        std::optional<std::string> password_hash
    );
    void LeaveRoom(const domain::RoomCode& code, const domain::ParticipantId& participant_id);
    std::vector<domain::Participant> GetRoomParticipants(const domain::RoomCode& code) const;
    bool ParticipantInRoom(const domain::ParticipantId& participant_id, const domain::RoomCode& room_code) const;

private:
    domain::RoomRepositoryPtr repository_;
};

} // namespace buzzweb::app
