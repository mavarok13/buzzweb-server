#include "buzzweb/domain/Room.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace buzzweb::domain {

Room::Room(RoomCode code, std::optional<RoomSecret> room_secret)
    : code_(std::move(code)), room_secret_(std::move(room_secret))
{
}

Room::~Room() = default;

const RoomCode& Room::GetCode() const
{
    return code_;
}

bool Room::IsSecretProtected() const
{
    return room_secret_.has_value();
}

bool Room::IsEmpty() const
{
    return participants_.empty();
}

JoinRoomResult Room::TryAddParticipant(const Participant& participant, std::optional<RoomSecret> room_secret)
{
    if (HasParticipant(participant.GetId())) {
        return JoinRoomResult::AlreadyInRoom;
    }
    if (room_secret_) {
        if (!room_secret) {
            return JoinRoomResult::SecretNotProvided;
        }
        if (*room_secret_ != *room_secret) {
            return JoinRoomResult::InvalidSecret; 
        }
    }

    participants_.push_back(participant);
    return JoinRoomResult::Joined;
}

LeaveRoomResult Room::RemoveParticipant(const ParticipantId& participant_id)
{
    if (!HasParticipant(participant_id)) {
        return LeaveRoomResult::NotInRoom;
    }
    
    participants_.erase(
        std::remove_if(
            participants_.begin(),
            participants_.end(),
            [&participant_id](const Participant& participant) {
                return participant.GetId() == participant_id;
            }
        ),
        participants_.end()
    );

    return LeaveRoomResult::Left;
}

bool Room::HasParticipant(const ParticipantId& participant_id) const
{
    return std::any_of(
        participants_.begin(),
        participants_.end(),
        [&participant_id](const Participant& participant) {
            return participant.GetId() == participant_id;
        }
    );
}

Participant& Room::FindParticipant(const ParticipantId& participant_id)
{
    auto participant = std::find_if(
        participants_.begin(),
        participants_.end(),
        [&participant_id](const Participant& current) {
            return current.GetId() == participant_id;
        }
    );

    if (participant == participants_.end()) {
        throw std::out_of_range("participant_not_found");
    }

    return *participant;
}

const Participant Room::FindParticipant(const ParticipantId& participant_id) const
{
    auto participant = std::find_if(
        participants_.begin(),
        participants_.end(),
        [&participant_id](const Participant& current) {
            return current.GetId() == participant_id;
        }
    );

    if (participant == participants_.end()) {
        throw std::out_of_range("participant_not_found");
    }

    return *participant;
}

std::vector<Participant> Room::GetParticipants() const
{
    return participants_;
}

} // namespace buzzweb::domain
