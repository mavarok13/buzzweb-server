#include "buzzweb/domain/Room.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace buzzweb::domain {

Room::Room(RoomCode code, std::optional<std::string> password_hash)
    : code_(std::move(code)), password_hash_(std::move(password_hash))
{
}

Room::~Room() = default;

const RoomCode& Room::GetCode() const
{
    return code_;
}

const std::optional<std::string>& Room::GetPasswordHash() const
{
    return password_hash_;
}

bool Room::IsPasswordProtected() const
{
    return password_hash_.has_value();
}

bool Room::IsEmpty() const
{
    return participants_.empty();
}

void Room::AddParticipant(const Participant& participant)
{
    if (HasParticipant(participant.GetId())) {
        throw std::invalid_argument("participant_already_exists");
    }

    participants_.push_back(participant);
}

void Room::RemoveParticipant(const ParticipantId& participant_id)
{
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
