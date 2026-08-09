#pragma once

#include "buzzweb/domain/Participant.hpp"
#include "buzzweb/domain/Room.hpp"
#include "buzzweb/domain/RoomRepository.hpp"
#include "buzzweb/app/Result.hpp"

#include <optional>
#include <string>
#include <vector>

namespace buzzweb::app {

struct RoomServiceError {
public:
    enum Type {
        WrongPassword,
        RoomNotFound,
        RoomCodeGenerationFailed,
        ParticipantUnavailable,
        NotInRoom,
        AlreadyJoined,
        RoomAlreadyExists,
        PasswordNotProvided,
        InternalError
    };

    Type type;
    std::string msg;
};

struct ParticipantJoinedRoom {
    domain::RoomCode room_code;
    domain::Participant participant;
    std::vector<domain::Participant> room_participants;
};
struct ParticipantLeftRoom {
    domain::RoomCode room_code;
    domain::ParticipantId participant_id;
    std::vector<domain::Participant> remaining_participants;
};

using ParticipantJoinedRoomResult = Result<ParticipantJoinedRoom, RoomServiceError>;
using ParticipantLeftRoomResult = Result<ParticipantLeftRoom, RoomServiceError>;
using ParticipantsLeftRoomResults = std::vector<ParticipantLeftRoomResult>;
using CreateRoomResult = Result<domain::Room, RoomServiceError>;
using GetParticipantsResult = Result<std::vector<domain::Participant>, RoomServiceError>;

class RoomService {
public:
    explicit RoomService(domain::RoomRepositoryPtr repository);
    ~RoomService();

    CreateRoomResult CreateRoom(std::optional<domain::RoomSecret> password_hash);
    ParticipantJoinedRoomResult JoinRoom(
        const domain::RoomCode& code,
        domain::Participant participant,
        std::optional<domain::RoomSecret> password_hash
    );
    ParticipantLeftRoomResult LeaveRoom(const domain::RoomCode& code, const domain::ParticipantId& participant_id);
    ParticipantsLeftRoomResults LeaveAllRooms(const domain::ParticipantId& participant_id);
    GetParticipantsResult GetRoomParticipants(const domain::RoomCode& code) const;
    bool ParticipantInRoom(
        const domain::ParticipantId& participant_id,
        const domain::RoomCode& room_code
    ) const;

private:
    domain::RoomRepositoryPtr repository_;
};

} // namespace buzzweb::app
