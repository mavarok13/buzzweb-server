# Roadmap

This file tracks planned implementation work and architecture decisions for `buzzweb-server`. Update it when priorities, contracts, or known future directions change.

## Completed Decisions

- 2026-07-08: Added executable/runtime wiring with environment-based WSS config and initial participant-event delivery through a dispatcher callback and session registry.
- 2026-06-26: Added minimal Boost.Asio/Beast WSS networking implementations for `Server`, `Listener`, `Session`, and `SessionRegistry`, wired into the static library.
- 2026-06-25: Added initial `src/` implementations for the domain and application layers, including thread-safe `InMemoryRoomRepository`, `RoomService`, `ControlDispatcher`, and a compiled static CMake library target.
- 2026-06-25: Decided that `RoomRepository` implementations must be thread-safe, with MVP `InMemoryRoomRepository` using one repository-level mutex and `Update()` using a transactional copy-then-commit workflow.
- 2026-06-24: Finalized the initial room-control JSON protocol shape for `create_room`, `join_room`, `leave_room`, `participant_joined`, `participant_left`, and generic request error responses. The canonical examples now live in `AGENTS.md`.
- 2026-06-24: Clarified that `buzzweb-server` is a signaling-only server. WebRTC media transport belongs to clients and external infrastructure; this server only relays opaque offer/answer/ICE signaling payloads.

## Near Term

- Keep `RoomService` as the only application layer that mutates rooms through `RoomRepository`.
- Add typed application result types for room use cases, such as `CreateRoomResult`, `JoinRoomResult`, `LeaveRoomResult`, and `ListParticipantsResult`.
- Replace string-based service errors with stable status enums, such as `RoomNotFound`, `WrongPassword`, `AlreadyJoined`, and `RoomFull`.
- Define participant identity lifecycle: a session may have a `ParticipantId` before joining, while a `Participant` is created for a room during successful create/join flow.
- Decide whether `LeaveRoom` should remove empty rooms inside a repository-level operation or through a repository API that can erase safely after update.

## MVP Signaling

- Implement the finalized initial JSON protocol schemas for `create_room`, `join_room`, and `leave_room`.
- Implement `ControlDispatcher` as the JSON routing layer that parses payloads, creates `Participant` values when needed, and calls `RoomService`.
- Map typed `RoomService` results to stable JSON response and error codes.
- Refine the runnable executable/config layer around the existing library-level WSS server.
- Keep `create_room`, `join_room`, and `leave_room` direct response flow working over WSS.
- Remove rooms when they become empty.
- Add basic 1-to-1 room capacity rules for the first calling MVP.
- Add optional password verification flow.
- Implement offer/answer/ICE relay between participants without parsing, validating, or terminating WebRTC media.
- Refine `participant_joined` and `participant_left` event payloads to match the finalized protocol examples exactly.
- Add basic logging for server lifecycle, sessions, room operations, and protocol errors.

## Verification And Quality

- Add a test framework such as GoogleTest or Catch2.
- Unit-test `Room::AddParticipant`, `Room::RemoveParticipant`, `Room::HasParticipant`, and participant lookup behavior.
- Unit-test `InMemoryRoomRepository::Add`, `FindByCode`, `Update`, `Remove`, and duplicate room handling.
- Unit-test `RoomService` use cases, including room not found, wrong password, already joined, room full, and success paths.
- Add protocol parsing and validation tests once message schemas are finalized.
- Add tests for dispatcher result-to-JSON mapping after typed service results exist.
- Add CI after the build is reproducible.
- Add formatting and static-analysis rules when source files exist.

## Deployment

- Add runtime configuration through environment variables or config files.
- Decide whether TLS is terminated inside the app or by a reverse proxy.
- Add `docker-compose.yml` for the signaling server and `coturn`.
- Document required ports for WSS and STUN/TURN.
- Add TURN/STUN URL configuration that can be advertised to clients.
- Add health/readiness endpoint if HTTP support is added beside WebSocket.

## Product Ideas

- Browser client using native WebRTC APIs.
- Native desktop client using `libdatachannel` or `libwebrtc` plus PortAudio/miniaudio.
- Room session tokens to allow reconnects.
- TURN server credentials and short-lived ICE server configuration for clients.
- Video support after 1-to-1 voice works.
- Group calls through an SFU such as Janus, LiveKit, Jitsi, mediasoup, or a dedicated custom SFU decision later.

## Engineering Ideas

- Keep WebRTC media out of this repository unless the project scope changes from signaling server to client/media server.
- Do not add `libwebrtc`, `libdatachannel`, RTP/RTCP handling, SFU, MCU, or media-relay behavior to the server unless the project scope explicitly changes.
- If group calls become required, evaluate SFU integration before designing custom media forwarding.
- Consider Redis only when multi-process or distributed room state is needed.
- Consider PostgreSQL only when rooms/users/history must survive process restarts.
