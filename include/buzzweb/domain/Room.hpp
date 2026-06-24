#pragma once

#include "buzzweb/domain/Participant.hpp"

#include <optional>
#include <string>
#include <vector>

namespace buzzweb::domain {

using RoomCode = std::string;

class Room {
public:
    Room(RoomCode code, std::optional<std::string> password_hash);
    ~Room();

    const RoomCode& GetCode() const;
    const std::optional<std::string>& GetPasswordHash() const;
    bool IsPasswordProtected() const;
    bool IsEmpty() const;

    void AddParticipant(const Participant& participant);
    void RemoveParticipant(const ParticipantId& participant_id);
    bool HasParticipant(const ParticipantId& participant_id) const;
    Participant & FindParticipant(const ParticipantId& participant_id);
    const Participant FindParticipant(const ParticipantId& participant_id) const;
    std::vector<Participant> GetParticipants() const;

private:
    RoomCode code_;
    std::optional<std::string> password_hash_;
    std::vector<Participant> participants_;
};

} // namespace buzzweb::domain
