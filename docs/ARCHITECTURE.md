# Architecture

`buzzweb-server` is intended to be a C++20 WebSocket/WSS signaling server for voice/video call rooms. The server should manage rooms, participants, and signaling messages. Real-time audio/video media should use WebRTC directly between clients for the first version.

## Current State

The repository currently contains only declarations:

- Public headers live under `include/buzzweb/`.
- There are no method definitions.
- There is no `src/` directory.
- There is no executable target.
- The CMake target is an `INTERFACE` target that carries include paths, C++20 requirements, and dependency links.

## Intended Layering

### Domain

Files: `include/buzzweb/domain/`

- `Participant` represents a user inside a room.
- `Room` owns room identity/rules and room participants.
- `RoomRepository` abstracts room storage.

Domain rules:

- Domain must not depend on Boost, WebSocket, JSON, OpenSSL, WebRTC, SDP, or ICE.
- `Room` should store the room code and optional password hash because those are room identity/rule data.
- `Room` currently stores participants in `std::vector<Participant>` because MVP rooms are expected to be small.
- `Participant` currently stores only ID and display name. Connection state belongs in the network/session layer unless a later feature proves otherwise.

### Application

Files: `include/buzzweb/app/`

- `RoomService` is the intended use-case layer for `CreateRoom`, `JoinRoom`, `LeaveRoom`, and `ListParticipants`.
- `ControlDispatcher` is the intended bridge between JSON control messages and application services.

Application rules:

- `RoomService` should be the only layer that mutates rooms through `RoomRepository`.
- Network classes should call `ControlDispatcher` or `RoomService`, not mutate `Room` directly.
- If repository methods return room copies, `RoomService` must save after every mutation.
- If repository methods later expose mutable access, pointers/references must not be stored long-term in sessions.

### Network

Files: `include/buzzweb/net/`

- `Server` is intended to own IO context, TLS context, listener, session registry, dispatcher reference, and logging.
- `Listener` is intended to accept TCP/TLS connections and create sessions.
- `Session` is intended to own one TLS WebSocket connection and handle inbound/outbound messages.
- `SessionRegistry` is intended to track active sessions by participant ID.

Network rules:

- WebSocket/WSS is for control and signaling only.
- Do not send microphone audio or video frames over WebSocket for the product MVP.
- The network layer may use Boost.Asio, Boost.Beast, OpenSSL, and strings/JSON protocol data.

## Intended Runtime Flow

1. Server starts and configures logging and TLS.
2. Listener accepts a TCP connection.
3. Session performs TLS handshake and WebSocket accept.
4. Session receives JSON control messages.
5. ControlDispatcher parses/routes messages by type.
6. RoomService performs room use cases and persists state through RoomRepository.
7. ControlDispatcher returns responses or relay events.
8. Session sends JSON responses/events to clients.
9. Clients use relayed offer/answer/ICE messages to establish WebRTC media directly.

## Intended Signaling Messages

Planning sessions identified these message types as likely protocol operations:

- `create_room`
- `join_room`
- `leave_room`
- `offer`
- `answer`
- `ice_candidate`
- `call_started`
- `call_ended`
- `participant_joined`
- `participant_left`

Names and payload schemas are not implemented or finalized yet.

## WebRTC Boundary

Use WebRTC for:

- Microphone audio.
- Future video.
- Encryption of media.
- Packet loss handling.
- Jitter buffering.
- NAT traversal with ICE/STUN/TURN.

Use WebSocket/WSS for:

- Room creation/join/leave.
- Authentication/session tokens when added.
- WebRTC offer/answer relay.
- ICE candidate relay.
- Participant and call-state events.

## Storage Model

Current storage is only an interface. No implementation exists yet.

Planned MVP storage:

- `InMemoryRoomRepository` as the source of truth for temporary call rooms.
- Rooms disappear on process restart.
- Repository storage may use `std::unordered_map<RoomCode, Room>`.
- Room participants can remain a `std::vector<Participant>`.

Future storage options:

- Redis for temporary distributed room/session state.
- PostgreSQL for persistent users/rooms if the product later needs accounts or history.
- SQLite for simple single-host persistence.

## Concurrency Considerations

The sessions identified a future risk with copy-and-save repository workflows:

- User 1 loads a room copy.
- User 2 loads the same room copy.
- User 1 saves changes.
- User 2 saves an older copy and overwrites User 1 changes.

MVP options to avoid this:

- Run room operations on one thread or one Asio strand.
- Add locking inside `InMemoryRoomRepository`.
- Add repository-level update methods.

Later options:

- Database transactions.
- Version fields or optimistic locking.
- Per-room synchronization.

## Important Constraints

- The current code intentionally has declarations without definitions.
- Adding `.cpp` implementations will require converting CMake from pure `INTERFACE` usage to a compiled target or adding an executable that compiles implementation files.
- Keep media handling out of this server until there is an explicit feature decision to build an SFU or media relay.
- For 1-to-1 calls, peer-to-peer WebRTC is enough for the intended MVP.
- For group calls, plan for an SFU later rather than trying to relay media over WebSocket.
