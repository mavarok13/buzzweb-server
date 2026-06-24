# Roadmap

This file tracks planned implementation work and architecture decisions for `buzzweb-server`. Update it when priorities, contracts, or known future directions change.

## Completed Decisions

- 2026-06-24: Finalized the initial room-control JSON protocol shape for `create_room`, `join_room`, `leave_room`, `participant_joined`, `participant_left`, and generic request error responses. The canonical examples now live in `AGENTS.md`.
- 2026-06-24: Clarified that `buzzweb-server` is a signaling-only server. WebRTC media transport belongs to clients and external infrastructure; this server only relays opaque offer/answer/ICE signaling payloads.

## Near Term

- Add `src/` and provide implementations for the current domain and application declarations.
- Default simple destructors in headers or define them in `.cpp` files to avoid linker errors.
- Convert CMake from the current declarations-only `INTERFACE` target to a compiled library plus executable, or choose another explicit structure before adding `.cpp` files.
- Add `InMemoryRoomRepository` as the first room storage implementation and source of truth for temporary rooms.
- Clarify the `RoomRepository::Update()` contract: decide whether the updater only mutates, returns a status, or uses a transactional copy-then-commit workflow.
- Keep `RoomService` as the only application layer that mutates rooms through `RoomRepository`.
- Add typed application result types for room use cases, such as `CreateRoomResult`, `JoinRoomResult`, `LeaveRoomResult`, and `ListParticipantsResult`.
- Replace string-based service errors with stable status enums, such as `RoomNotFound`, `WrongPassword`, `AlreadyJoined`, and `RoomFull`.
- Define participant identity lifecycle: a session may have a `ParticipantId` before joining, while a `Participant` is created for a room during successful create/join flow.
- Add a minimal `main.cpp` that wires repository, service, dispatcher, and server configuration after the compiled target structure is chosen.

## MVP Signaling

- Implement the finalized initial JSON protocol schemas for `create_room`, `join_room`, and `leave_room`.
- Implement `ControlDispatcher` as the JSON routing layer that parses payloads, creates `Participant` values when needed, and calls `RoomService`.
- Map typed `RoomService` results to stable JSON response and error codes.
- Implement WebSocket/WSS accept and read/write loops.
- Implement `create_room`, `join_room`, and `leave_room` control flow.
- Remove rooms when they become empty.
- Add basic 1-to-1 room capacity rules for the first calling MVP.
- Add optional password verification flow.
- Implement offer/answer/ICE relay between participants without parsing, validating, or terminating WebRTC media.
- Add `participant_joined` and `participant_left` events.
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
