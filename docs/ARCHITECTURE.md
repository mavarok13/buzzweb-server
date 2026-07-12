# Architecture

`buzzweb-server` is intended to be a C++20 WebSocket/WSS signaling server for voice/video call rooms. The server manages rooms, participants, and signaling messages only. It is not a WebRTC media server; real-time audio/video media should use WebRTC directly between clients for the first version.

## Current State

The repository currently contains initial domain, application, and minimal WS/WSS networking implementations:

- Public headers live under `include/buzzweb/`.
- Domain and application method definitions live under `src/domain/` and `src/app/`.
- `InMemoryRoomRepository` is available as the first concrete room repository.
- There is an executable target that wires the in-memory repository, room service, dispatcher, session registry, environment config, and WS/WSS server.
- The CMake configuration builds a compiled static library plus the `buzzweb_server` executable.
- Network-layer method definitions exist for server ownership, TCP accept, plain WebSocket and TLS WebSocket session startup, JSON control dispatch, response writes, session registry storage, structured participant-event delivery, and targeted WebRTC signaling relay.
- Empty rooms are removed through repository-level conditional cleanup after successful leave-room operations. Leave/disconnect runtime cleanup uses prepared structured result/event data instead of rereading room state after mutation.
- A standalone browser demo exists at `client_demo/index.html` for manual two-tab WebSocket/WSS signaling and peer-to-peer WebRTC media testing.

## Intended Layering

### Domain

Files: `include/buzzweb/domain/`

- `Participant` represents a user inside a room.
- `Room` owns room identity/rules and room participants.
- `RoomRepository` abstracts room storage.
- `InMemoryRoomRepository` stores rooms in memory behind one repository-level mutex.

Domain rules:

- Domain must not depend on Boost, WebSocket, JSON, OpenSSL, WebRTC, SDP, or ICE.
- `Room` should store the room code and optional password hash because those are room identity/rule data.
- `Room` currently stores participants in `std::vector<Participant>` because MVP rooms are expected to be small.
- `Participant` currently stores only ID and display name. Connection state belongs in the network/session layer unless a later feature proves otherwise.

### Application

Files: `include/buzzweb/app/`

- `RoomService` is the intended use-case layer for `CreateRoom`, `JoinRoom`, `LeaveRoom`, and `ListParticipants`.
- `ControlDispatcher` is the intended bridge between JSON control/signaling messages and application services, emits structured participant-event messages through a callback supplied by runtime wiring, and emits targeted relay messages through a delivery callback.

Application rules:

- `RoomService` should be the only layer that mutates rooms through `RoomRepository`.
- Network classes should call `ControlDispatcher` or `RoomService`, not mutate `Room` directly.
- `RoomService` mutates rooms through `RoomRepository::Update()` so joins/leaves use repository-level transactional updates.
- `RoomService` uses `RoomRepository::RemoveIfEmpty()` after successful leave operations so cleanup checks and removal happen under the repository lock.
- `RoomService` exposes participant membership checks so signaling relay can verify sender and target room membership before delivery.
- If repository methods later expose mutable access, pointers/references must not be stored long-term in sessions.

### Network

Files: `include/buzzweb/net/`

- `Server` owns IO context, TLS context, listener, dispatcher reference, TLS-mode configuration, and basic logging setup; the executable owns `SessionRegistry` and passes it into the server/runtime event bridge.
- `Listener` accepts TCP connections and creates `SslSession` when TLS is enabled or `PlainSession` when TLS is disabled.
- `SessionBase` owns shared per-session state and behavior: participant identity, dispatcher/registry access, JSON control-message handling, registry cleanup, and the virtual `Start()`, `Send()`, and `Close()` interface.
- `PlainSession` owns one plain WebSocket connection and starts directly with WebSocket accept.
- `SslSession` owns one TLS WebSocket connection and performs TLS handshake before WebSocket accept.
- `SessionRegistry` tracks active `SessionBase` instances by participant ID behind a mutex.

Network rules:

- WebSocket/WSS is for control and signaling only.
- Do not send microphone audio or video frames over WebSocket for the product MVP.
- Treat SDP offers, SDP answers, and ICE candidates as opaque signaling payloads to relay; do not parse or terminate media in this server.
- The network layer may use Boost.Asio, Boost.Beast, OpenSSL, and strings/JSON protocol data.
- Keep control-message parsing and response/event handling shared in `SessionBase`; only handshake and lowest-layer close behavior should differ between plain WS and TLS WSS sessions.

## Intended Runtime Flow

1. Server starts, configures logging, reads TLS mode, and configures TLS only when enabled.
2. Listener accepts a TCP connection.
3. Listener creates either `SslSession` or `PlainSession` based on TLS mode.
4. `SslSession` performs TLS handshake then WebSocket accept; `PlainSession` starts directly with WebSocket accept.
5. Session receives JSON control messages.
6. ControlDispatcher parses/routes messages by type.
7. RoomService performs room use cases and persists state through RoomRepository.
8. ControlDispatcher returns structured responses for create, join, and leave room requests.
9. Session sends JSON responses to clients.
10. ControlDispatcher emits structured participant events through a callback; runtime wiring sends event JSON to recipients from the prepared participant list through `SessionRegistry`.
11. For `offer`, `answer`, and `ice_candidate`, ControlDispatcher validates sender and target room membership, emits a `ControlRelay`, and Session runtime delivery sends the relay event to the target participant's active session.
12. Clients use relayed offer/answer/ICE messages to establish WebRTC media directly; the server does not join the media path.

Structured participant events and WebRTC offer/answer/ICE relay events exist. `participant_joined` carries the joined participant object and current participants list; `participant_left` carries the departed participant ID and remaining participants list.

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

Initial room-control schemas are now finalized for `create_room`, `join_room`, `leave_room`, `participant_joined`, `participant_left`, generic request errors, and WebRTC signaling relay messages. The canonical request/response/event JSON examples live in `AGENTS.md`.

## WebRTC Boundary

`buzzweb-server` is WebRTC-aware only at the signaling boundary. It may route SDP offers, SDP answers, and ICE candidates as JSON data, but it should not link a WebRTC media library, decode media, receive RTP/RTCP packets, or behave as an SFU/MCU/media relay.

`client_demo/index.html` owns the browser-side `RTCPeerConnection`, local media capture, remote media playback, SDP creation/answering, and ICE candidate application. The server remains a signaling relay only and never receives microphone or camera media frames from the demo.

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

Out of scope for this server:

- Audio/video capture, encoding, decoding, mixing, and playback.
- RTP/RTCP packet handling.
- WebRTC peer connection ownership.
- SFU, MCU, or media-relay behavior unless the project scope is explicitly changed later.

## Storage Model

Current storage has an interface and one in-memory implementation.

MVP storage:

- `InMemoryRoomRepository` as the source of truth for temporary call rooms.
- Rooms disappear on process restart.
- Repository storage uses `std::unordered_map<RoomCode, Room>`.
- Room participants can remain a `std::vector<Participant>`.

Future storage options:

- Redis for temporary distributed room/session state.
- PostgreSQL for persistent users/rooms if the product later needs accounts or history.
- SQLite for simple single-host persistence.

## Concurrency Considerations

The project should avoid unsafe load-copy-save room mutations:

- User 1 loads a room copy.
- User 2 loads the same room copy.
- User 1 saves changes.
- User 2 saves an older copy and overwrites User 1 changes.

MVP decision:

- `RoomRepository` implementations must make all public methods thread-safe.
- `InMemoryRoomRepository` should start with one repository-level mutex, not one mutex per room.
- `RoomRepository::Update()` should be a transactional copy-then-commit operation: lock repository state, find the stored room, copy it, run the updater on the copy, and replace stored state only when the updater reports success.
- `RoomRepository::Remove()` must be synchronized with `Update()` so remove and update cannot race for the same room.
- `Room` should remain a domain value object without an internal mutex for the MVP.
- Updaters must not call back into the same repository while `Update()` is running.

Per-room mutexes are a later optimization, not the MVP default. They require explicit lifetime and erase rules because `Remove()` can otherwise race with an in-progress per-room update.

Later options:

- Database transactions.
- Version fields or optimistic locking.
- Per-room synchronization.

## Important Constraints

- The current code has a runnable executable entry point wired to environment variables for TLS mode, TLS certificate path, private key path, and port. TLS is default-on; plain WS is available for local/non-TLS deployments by disabling TLS explicitly.
- Event payloads and runtime ownership should continue to be covered by typed results, logging, and protocol tests as they are added.
- Keep media handling out of this server until there is an explicit feature decision to build an SFU or media relay.
- For 1-to-1 calls, peer-to-peer WebRTC is enough for the intended MVP.
- For group calls, plan for an SFU later rather than trying to relay media over WebSocket.
