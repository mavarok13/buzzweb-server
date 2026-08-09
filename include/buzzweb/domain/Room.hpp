#pragma once

#include "buzzweb/domain/Participant.hpp"

#include <optional>
#include <string>
#include <vector>

namespace buzzweb::domain {

using RoomCode = std::string;
using RoomSecret = std::string;

enum JoinRoomResult {
    Joined,
    InvalidSecret,
    SecretNotProvided,
    AlreadyInRoom
};
enum LeaveRoomResult {
    Left,
    NotInRoom
};

class Room {
public:
    Room(RoomCode code, std::optional<RoomSecret> room_secret = std::nullopt);
    ~Room();

    const RoomCode& GetCode() const;
    bool IsSecretProtected() const;
    bool IsEmpty() const;

    JoinRoomResult TryAddParticipant(const Participant& participant, std::optional<RoomSecret> room_secret);
    LeaveRoomResult RemoveParticipant(const ParticipantId& participant_id);
    bool HasParticipant(const ParticipantId& participant_id) const;
    Participant & FindParticipant(const ParticipantId& participant_id);
    const Participant FindParticipant(const ParticipantId& participant_id) const;
    std::vector<Participant> GetParticipants() const;

private:
    RoomCode code_;
    std::optional<RoomSecret> room_secret_;
    std::vector<Participant> participants_;
};

} // namespace buzzweb::domain
