#include "buzzweb/net/SessionRegistry.hpp"

#include "buzzweb/net/Session.hpp"

#include <utility>

namespace buzzweb::net {

SessionRegistry::SessionRegistry() = default;

SessionRegistry::~SessionRegistry() = default;

void SessionRegistry::Add(SessionPtr session)
{
    if (!session) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session->GetParticipantId()] = std::move(session);
}

void SessionRegistry::Remove(const domain::ParticipantId& participant_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(participant_id);
}

std::optional<SessionPtr> SessionRegistry::Find(const domain::ParticipantId& participant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto session = sessions_.find(participant_id);
    if (session == sessions_.end()) {
        return std::nullopt;
    }

    return session->second;
}

std::vector<domain::ParticipantId> SessionRegistry::GetParticipantIds() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::ParticipantId> participant_ids;
    participant_ids.reserve(sessions_.size());

    for (const auto& [participant_id, session] : sessions_) {
        participant_ids.push_back(participant_id);
    }

    return participant_ids;
}

} // namespace buzzweb::net
