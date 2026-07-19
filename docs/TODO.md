# 📝 TO DO 📝

## Near tasks
❌ Add result struct with variant for room repository. Adapt RoomService, ControlDispatcher for this

❌ Move JSON protocol codec from Session and ControlDispatcher in separated class

✅ Complete event JSON shape: add remaining participants

❌ src/app/ControlDispatcher.cpp:205-210, 220-225: если event_handler_ бросит после send(SuccessResponse(...)), dispatcher поймает исключение внешним catch и может отправить второй response на тот же request_id.
Сейчас это маловероятно, потому что main.cpp больше не ходит в GetRoomParticipants(), но архитектурно риск остался. Лучше event delivery ошибки не должны превращать успешный leave_room/join_room в второй ответ. Можно позже обернуть event handler отдельно.

✅ Add exception handler for `RoomService::LeaveAllRooms`

❌ One args order for repository, service: room_code, participant_id, etc. `bool ParticipantInRoom(const domain::ParticipantId& participant_id, const domain::RoomCode& room_code) const;`

✅ Move args of `ControlEventHandler` in struct `ControlEvent`

❌ All in unity form: room -> room_code, id (participant) -> participant_id, etc

✅ Make a choice
```
struct ParticipantLeftRoom {
        domain::RoomCode room_code;
        domain::Participant participant; <-------------> domain::ParticipantId participant_id;
        std::vector<domain::Participant> remaining_participants;
    };
```

❌ Fix this shit
```
buzzweb::app::ControlDispatcher dispatcher(room_service, [&room_service, &registry] (const buzzweb::app::ControlEventData& event, const std::vector<buzzweb::domain::Participant>& participants) {
        for (const auto participant : participants) {
            if (participant.GetId() == participant_id) {
                continue;
            }

            auto session_ptr_opt = registry.Find(participant.GetId());
            if (session_ptr_opt) {

                std::string event_type;
                if (event.type == buzzweb::app::ControlEventType::Joined) {
                    event_type = "participant_joined";
                } else if (event.type == buzzweb::app::ControlEventType::Left) {
                    event_type = "participant_left";
                } else {
                    return;
                }

                nlohmann::json event_message = {
                    {"type", event_type},
                    {"payload", event.payload}
                }

                (*session_ptr_opt)->Send(event_message.dump());
            }
        }
    });
```