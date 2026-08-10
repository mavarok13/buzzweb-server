#include "buzzweb/domain/Participant.hpp"

#include <utility>

namespace buzzweb::domain {

Participant::Participant(ParticipantId id, std::string name)
    : id_(std::move(id)), name_(std::move(name))
{
}

Participant::~Participant() = default;

const ParticipantId& Participant::GetId() const
{
    return id_;
}

const std::string& Participant::GetName() const
{
    return name_;
}

void Participant::SetName(std::string name)
{
    name_ = std::move(name);
}

} // namespace buzzweb::domain
