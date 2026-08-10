# AGENTS.md

Repository guidance for future coding agents. Start here before changing code.

## Read Order

1. Read this file.
2. Read `README.md` for user-facing installation, runtime, and contribution instructions.
3. Read `docs/INFO.md` for the short project summary and current feature state.
4. Read `docs/STACK.md` for language, libraries, build, deploy, and runtime notes.
5. Read `docs/ARCHITECTURE.md` for planned component boundaries and data flow.
6. Read `docs/ROADMAP.md` for planned work and known future directions.
7. Read `docs/LAST_CHANGES.md` and `docs/CHANGELOGS.md` before editing, so new work continues from the latest context.
8. Read `session-ses_115d.md` for planning context and `session-ses_114d.md` for the initial development context when making architecture-level changes.

This is useful, but not guaranteed automatically by every agent runtime. Agents that support repository instructions often auto-load `AGENTS.md`; agents that do not should be explicitly told to read it.

## Documentation Maintenance Rule

After any project change, update the context docs when relevant:

- Update `docs/INFO.md` if user-visible features, behavior, requirements, or known limitations change.
- Update `docs/STACK.md` if language version, dependencies, deployment, environment variables, or infrastructure change.
- Update `docs/ARCHITECTURE.md` if component responsibilities, data flow, storage model, or runtime flow change.
- Update `docs/ROADMAP.md` if planned work, priorities, or known future directions change.
- Update `docs/LAST_CHANGES.md` with the newest concise summary.
- Append a dated entry to `docs/CHANGELOGS.md` for every completed change, including documentation-only changes.
- Keep `README.md` installation, runtime configuration, and contribution instructions aligned with the actual build and development workflow.

Keep changelog entries newest first. Mention files or areas touched, why they changed, and any verification done.

## Optional Codebase Index

If the `codebase-index` tool is available and the local index is fresh, use it before broad manual scans for architecture, symbol, reference, impact, or data-flow questions:

```sh
codebase-index explain "architecture overview" --token-budget 3000 --json
codebase-index search "query" --json
codebase-index symbol "SymbolName" --json
codebase-index refs "SymbolName" --json
codebase-index impact "path/or/symbol" --json
```

If the index is missing, run `codebase-index index`. If it is stale, run `codebase-index update` for small changes or `codebase-index index` for a full rebuild. Always verify important conclusions in the source files before editing.

## Project Map

- `README.md` is the user-facing entry point for installation, local/Docker startup, runtime configuration, and contribution instructions.
- `CMakeLists.txt` defines the compiled `buzzweb_server_lib` static library, `BuzzWeb::Server` alias, and `buzzweb_server` executable.
- `Dockerfile` defines a Debian 12 build environment with CMake, Ninja, Boost, OpenSSL, and `nlohmann_json` packages.
- `include/buzzweb/domain/` contains room-domain declarations: `Room`, `Participant`, `RoomRepository`, and `InMemoryRoomRepository`.
- `include/buzzweb/app/` contains application-layer declarations: `RoomService` and `ControlDispatcher`.
- `src/domain/` contains implementations for domain value objects and `InMemoryRoomRepository`.
- `src/app/` contains implementations for room use cases and control-message dispatch.
- `include/buzzweb/net/` contains networking declarations: `Server`, `Listener`, `SessionBase`, `PlainSession`, `SslSession`, and `SessionRegistry`.
- `src/net/` contains the minimal Boost.Asio/Beast WS/WSS server, listener, session, and session-registry implementations.
- `session-ses_115d.md` contains planning context for WebRTC, WSS signaling, deployment, CMake, and domain/repository design.
- `session-ses_114d.md` contains the initial development session that created the current skeleton and naming conventions.
- `docs/` contains persistent project context for agents.

There is a runnable executable target. Domain, application, minimal WS/WSS networking, runtime config, and entry-point code have `.cpp` implementations under `src/`.

## Coding Guidelines

- Use C++20 and the existing CMake project `BuzzWebServer`.
- Preserve the current public include style: `#include "buzzweb/..."` from the `include/` root.
- Current method/function naming uses PascalCase, with examples like `CreateRoom()`, `FindByCode()`, `GetName()`, and `Start()`.
- Type aliases and classes use PascalCase, such as `RoomCode`, `ParticipantId`, `RoomRepository`, and `SessionRegistry`.
- Private fields currently use trailing underscores, such as `code_`, `participants_`, and `repository_`.
- Keep WebRTC media out of the server for the first phase. The server is for WebSocket/WSS control and signaling only.
- Do not send voice/video media over WebSocket except for explicit experiments; media should use WebRTC.
- Keep domain classes free of Boost, JSON, OpenSSL, WebSocket, SDP, ICE, and network-session details.
- Let `RoomService` own application-level room mutations. Network classes should call application services instead of modifying rooms directly.
- Known technical debt: room creation and automatic creator join are currently separate operations. If room creation succeeds but the subsequent join fails, an empty room can remain in the repository. A future fix should make the room visible only after the creator has been added, rather than relying on an unsafe compensating `Remove()`.
- Current `Room` stores participants in `std::vector<Participant>`, not `std::unordered_map`, because the MVP is expected to use small rooms.
- Current `Participant` is intentionally minimal: identity and display name only.
- Keep `RoomRepository` as a storage abstraction so in-memory, Redis, or PostgreSQL storage can be added later.
- `RoomRepository` implementations must be thread-safe for all public methods, including `Add()`, `FindByCode()`, `Update()`, `Remove()`, `RemoveIfEmpty()`, `Exists()`, `ParticipantInRoom()`, and `GetRoomCodes()`.
- For the MVP `InMemoryRoomRepository`, prefer one repository-level mutex over per-room mutexes. Per-room locking can be considered later only if contention becomes real and remove/update lifetime rules are designed explicitly.
- `RoomRepository::Update()` uses a transactional copy-then-commit workflow: lock repository state, find the stored room, copy it, run the updater on the copy, and replace stored state only when the updater reports success.
- `RoomRepository::Remove()` must be synchronized with `Update()` so a room cannot be removed while an update is being evaluated or committed.
- `RoomRepository::RemoveIfEmpty()` is the MVP cleanup primitive for leave-room flow. It must check room emptiness and erase under the same repository lock, and it should be safe as an idempotent cleanup no-op when the room is already gone or no longer empty.
- Updaters return commit/abort status. If the updater aborts or fails validation, leave the stored room unchanged.
- Do not put mutexes inside `Room` for the MVP. Keep `Room` a domain value object and keep synchronization in the repository implementation.
- Updaters passed to `RoomRepository::Update()` must not call back into the same repository, because the repository may already hold its mutex.
- `RoomService` leave/disconnect cleanup should use the structured leave result data instead of rereading the room after mutation; event recipients should come from the participant list already prepared by the application/control layer.
- `RoomService::LeaveAllRooms()` should swallow only expected cleanup misses, currently `room_not_found` and `not_in_room`; unexpected errors should continue to propagate.
- Prefer small, direct changes that match the current early implementation stage.

## Runtime Configuration Guidance

- `BUZZWEB_TLS_ENABLED` controls whether the server accepts WSS or plain WS. TLS is enabled by default.
- Treat `BUZZWEB_TLS_ENABLED=0`, `BUZZWEB_TLS_ENABLED=false`, and `BUZZWEB_TLS_ENABLED=FALSE` as disabling TLS; other values should keep TLS enabled.
- When TLS is enabled, `BUZZWEB_CERTIFICATE_FILE_PATH` and `BUZZWEB_PRIVATE_KEY_PATH` are required and loaded into the server TLS context.
- When TLS is disabled, certificate and private-key environment variables are not required; the listener should create plain WebSocket sessions.
- `BUZZWEB_SERVER_PORT` remains the optional listen port override for both WS and WSS modes.

## Runtime Cleanup Guidance

- `SessionBase` owns shared session behavior such as participant identity, dispatcher access, registry cleanup, and message handling. Keep transport-specific handshake and lowest-layer close behavior in `PlainSession` or `SslSession`.
- `SessionRegistry` stores `std::shared_ptr<SessionBase>` so runtime delivery can target either plain WS or TLS WSS sessions through the same `Send()`/`Close()` interface.
- `Listener` should select `SslSession` when TLS is enabled and `PlainSession` when TLS is disabled. Avoid duplicating control-message handling between the two session modes.
- `SessionBase::RemoveFromRegistry()` is part of connection-close cleanup. It should log cleanup exceptions and keep the close path moving rather than letting cleanup failures break session shutdown.
- Runtime room-event delivery should use the participant recipient list supplied in the structured event data. Avoid rereading room state from `main.cpp` after leave/disconnect just to discover recipients.

## JSON Protocol Guidance

Initial room-control JSON schemas are decided for the MVP. Keep protocol changes compatible with this shape unless the user explicitly chooses a new schema.

Client requests use a stable envelope with `type`, `request_id`, and `payload`:

```json
{
  "type": "create_room",
  "request_id": "req-001",
  "payload": {}
}
```

Direct responses include the same `request_id`, an `ok` boolean, and either `payload` or `error`:

```json
{
  "type": "create_room_result",
  "request_id": "req-001",
  "ok": true,
  "payload": {}
}
```

Room events do not need `request_id` because they are pushed asynchronously:

```json
{
  "type": "participant_joined",
  "payload": {}
}
```

`create_room` should create the room and join the creator automatically:

```json
{
  "type": "create_room",
  "request_id": "req-001",
  "payload": {
    "display_name": "Alice",
    "password": "optional-room-password"
  }
}
```

```json
{
  "type": "create_room_result",
  "request_id": "req-001",
  "ok": true,
  "payload": {
    "room_code": "742913",
    "participant": {
      "participant_id": "p_01JZABC123",
      "display_name": "Alice"
    },
    "participants": [
      {
        "participant_id": "p_01JZABC123",
        "display_name": "Alice"
      }
    ]
  }
}
```

`join_room` requires `room_code` and `display_name`; `password` is optional:

```json
{
  "type": "join_room",
  "request_id": "req-002",
  "payload": {
    "room_code": "742913",
    "display_name": "Bob",
    "password": "optional-room-password"
  }
}
```

```json
{
  "type": "join_room_result",
  "request_id": "req-002",
  "ok": true,
  "payload": {
    "room_code": "742913",
    "participant": {
      "participant_id": "p_01JZDEF456",
      "display_name": "Bob"
    },
    "participants": [
      {
        "participant_id": "p_01JZABC123",
        "display_name": "Alice"
      },
      {
        "participant_id": "p_01JZDEF456",
        "display_name": "Bob"
      }
    ]
  }
}
```

Send `participant_joined` to existing room participants after a successful join:

```json
{
  "type": "participant_joined",
  "payload": {
    "room_code": "742913",
    "participant": {
      "participant_id": "p_01JZDEF456",
      "display_name": "Bob"
    },
    "participants": [
      {
        "participant_id": "p_01JZABC123",
        "display_name": "Alice"
      },
      {
        "participant_id": "p_01JZDEF456",
        "display_name": "Bob"
      }
    ]
  }
}
```

`participant_joined` is implemented as a structured participant event. The payload should include both the joined `participant` object and the full current `participants` list.

`leave_room` uses the calling session participant identity and the target room code:

```json
{
  "type": "leave_room",
  "request_id": "req-003",
  "payload": {
    "room_code": "742913"
  }
}
```

```json
{
  "type": "leave_room_result",
  "request_id": "req-003",
  "ok": true,
  "payload": {
    "room_code": "742913"
  }
}
```

Send `participant_left` to remaining room participants after a successful leave:

```json
{
  "type": "participant_left",
  "payload": {
    "room_code": "742913",
    "participant_id": "p_01JZDEF456",
    "participants": [
      {
        "participant_id": "p_01JZABC123",
        "display_name": "Alice"
      }
    ]
  }
}
```

`participant_left` is implemented as a structured participant event. The payload should include the departed `participant_id` and the remaining `participants` list.

Use one stable error shape for failed requests:

```json
{
  "type": "join_room_result",
  "request_id": "req-002",
  "ok": false,
  "error": {
    "code": "room_not_found",
    "message": "Room was not found."
  }
}
```

Initial stable error codes: `invalid_json`, `invalid_message`, `missing_field`, `invalid_field`, `room_not_found`, `wrong_password`, `already_joined`, `room_full`, `not_in_room`, `participant_unavailable`, and `internal_error`.

WebRTC signaling relay is implemented for `offer`, `answer`, and `ice_candidate`. Requests use the same envelope style and direct responses use `offer_result`, `answer_result`, and `ice_candidate_result`.

Signaling requests must include `room_code` and `target_participant_id`. The dispatcher verifies that the sender and target participant are both in the room before relaying. Runtime delivery then checks `SessionRegistry`; if the target has no active session, the sender receives `participant_unavailable`.

Example `offer` request:

```json
{
  "type": "offer",
  "request_id": "req-004",
  "payload": {
    "room_code": "742913",
    "target_participant_id": "p_01JZDEF456",
    "sdp": "opaque-client-sdp"
  }
}
```

Example success response to the sender:

```json
{
  "type": "offer_result",
  "request_id": "req-004",
  "ok": true,
  "payload": {
    "room_code": "742913"
  }
}
```

Example relay event delivered to the target participant:

```json
{
  "type": "offer",
  "payload": {
    "room_code": "742913",
    "target_participant_id": "p_01JZDEF456",
    "from_participant_id": "p_01JZABC123",
    "sdp": "opaque-client-sdp"
  }
}
```

Use the same shape for `answer` and `ice_candidate`; keep SDP and ICE payload data opaque to the server.

## Local Verification Policy

Local verification depends on installed dependencies.

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` was previously attempted and reached dependency discovery, but failed because Boost package config was not installed/configured on this Windows host.
- Docker CLI was previously present, but Docker Desktop's Linux engine was not running, so `docker build` could not be used for verification.
- If local Boost/OpenSSL/`nlohmann_json` packages or Docker are available, CMake/Docker verification is reasonable.
- Do not install system dependencies, start Docker services, or change package-manager state unless the user explicitly asks.
- If verification cannot run, verify by source inspection and clearly report the local dependency or Docker daemon limitation.
