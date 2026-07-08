#pragma once

#include "buzzweb/domain/Participant.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace buzzweb::net {

class Session;
using SessionPtr = std::shared_ptr<Session>;

class SessionRegistry {
public:
    SessionRegistry();
    ~SessionRegistry();

    void Add(SessionPtr session);
    void Remove(const domain::ParticipantId& participant_id);
    std::optional<SessionPtr> Find(const domain::ParticipantId& participant_id) const;
    std::vector<domain::ParticipantId> GetParticipantIds() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<domain::ParticipantId, SessionPtr> sessions_;
};

} // namespace buzzweb::net
