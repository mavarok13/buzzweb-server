#pragma once

#include <chrono>
#include <string>

namespace buzzweb::domain {

using ParticipantId = std::string;

class Participant {
public:
    Participant(ParticipantId id, std::string name);
    ~Participant();

    const ParticipantId& GetId() const;
    const std::string& GetName() const;

    void SetName(std::string name);

private:
    ParticipantId id_;
    std::string name_;
};

} // namespace buzzweb::domain
