# Changelogs

Keep newest entries first. Append an entry for every completed change, including documentation-only changes.

## 2026-07-08 - Runtime Event Wiring Build Fixes

- Added/finalized runtime wiring for the executable to own `SessionRegistry`, pass it into `net::Server`, and provide a `ControlDispatcher` event handler that sends room participant events to connected sessions.
- Fixed participant-event JSON construction by using `nlohmann::json::array()` for the event payload participants placeholder.
- Included `buzzweb/net/Session.hpp` in `src/main.cpp` so the runtime event handler can call `Session::Send()` on sessions returned from `SessionRegistry`.
- Updated project context docs to reflect the executable target, environment-based WSS configuration, and initial participant-event delivery.
- Verification: `docker build -t buzzweb-server .` completed successfully.

## 2026-07-08 - Dev Docker Helper Scripts

- Added `scripts/generate-dev-certs.sh` to create local self-signed development TLS files under ignored `certs/`.
- Added `scripts/docker-build.sh` to build the `buzzweb-server` Docker image from the repository root.
- Added `scripts/docker-run-dev.sh` to run the image with mounted dev certs and Git Bash/MSYS path conversion disabled for container paths.
- Verification: `bash -n` passed for all added scripts.

## 2026-07-08 - Docker Build SSL WebSocket Fix

- Added the missing Boost.Beast SSL websocket support include to `include/buzzweb/net/Session.hpp`.
- Fixed the Docker build failure where `boost::beast::websocket::stream<boost::asio::ssl::stream<...>>` could not find SSL teardown support during `Session.cpp` compilation.
- Verification: `docker build -t buzzweb-server .` completed successfully.

## 2026-06-26 - Minimal WSS Network Implementation

- Added `src/net/Server.cpp`, `src/net/Listener.cpp`, `src/net/Session.cpp`, and `src/net/SessionRegistry.cpp`.
- Implemented library-level WSS runtime ownership: `Server` owns IO/TLS/listener/registry, `Listener` accepts TCP sockets, and `Session` performs TLS handshake, WebSocket accept, text JSON reads, control dispatch, and JSON response writes.
- Added a thread-safe `SessionRegistry` keyed by generated participant IDs.
- Wired network sources into `CMakeLists.txt` and updated project context docs plus `AGENTS.md` to reflect that networking is no longer declarations-only.
- Deliberately did not add broadcasts, WebRTC offer/answer/ICE relay, media transport over WebSocket, or an executable/config CLI.
- Verification: `git diff --check` passed; `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` is blocked because Boost package config is unavailable on this Windows host.

## 2026-06-25 - Domain And App Implementations

- Added `src/domain/` implementations for `Participant`, `Room`, and the new `InMemoryRoomRepository`.
- Added `include/buzzweb/domain/InMemoryRoomRepository.hpp` as the first concrete room repository, with thread-safe public methods and transactional `Update()` commits.
- Added `src/app/RoomService.cpp` for room creation, joining, leaving, and participant listing through `RoomRepository`.
- Added `src/app/ControlDispatcher.cpp` for create/join/leave routing and structured `ControlResponse` results with `request_id`, `ok`, payload, and error data.
- Converted `CMakeLists.txt` from an `INTERFACE` library to a compiled static library target.
- Updated repository context docs and `AGENTS.md` to reflect the new source layout and remaining limitations.
- Verification: `git diff --check` passed; `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` is blocked because Boost package config is unavailable on this Windows host.

## 2026-06-25 - Thread-Safe Room Repository Contract

- Added `AGENTS.md` guidance that `RoomRepository` implementations must be thread-safe for all public methods.
- Recorded the MVP choice to use one repository-level mutex in `InMemoryRoomRepository` instead of per-room mutexes.
- Documented the `RoomRepository::Update()` transactional copy-then-commit workflow and the need for updater commit/abort status.
- Clarified that `RoomRepository::Remove()` must be synchronized with `Update()` and that `Room` should remain a mutex-free domain value object for the MVP.
- Updated `docs/ARCHITECTURE.md`, `docs/ROADMAP.md`, and `docs/LAST_CHANGES.md` with the decision.
- Verification: documentation inspection only.

## 2026-06-24 - Signaling-Only WebRTC Boundary

- Updated `docs/INFO.md`, `docs/STACK.md`, `docs/ARCHITECTURE.md`, and `docs/ROADMAP.md` to clarify that `buzzweb-server` is a signaling-only server, not a WebRTC media server.
- Documented that clients and external infrastructure own WebRTC media transport, while the server only relays opaque offer/answer/ICE signaling payloads.
- Explicitly kept RTP/RTCP handling, media encoding/decoding, peer connection ownership, SFU, MCU, and media-relay behavior out of server scope unless the project scope changes later.
- Updated `docs/LAST_CHANGES.md` with the new documentation summary.
- Verification: documentation inspection only.

## 2026-06-24 - Initial JSON Protocol Shape

- Added canonical room-control JSON request, response, event, and error examples to `AGENTS.md` for future agents.
- Marked the initial JSON protocol shape decision complete in `docs/ROADMAP.md` and left implementation as the remaining MVP signaling work.
- Updated `docs/INFO.md` and `docs/ARCHITECTURE.md` to reflect that the room-control schemas are finalized but not implemented.
- Verification: documentation inspection only.

## 2026-06-24 - Roadmap Rewrite

- Rewrote `docs/ROADMAP.md` in English around the current near-term implementation plan.
- Added roadmap coverage for `RoomRepository::Update()` contract decisions, typed `RoomService` results, participant identity lifecycle, dispatcher-to-service mapping, JSON schemas, verification, and deployment work.
- Updated `docs/LAST_CHANGES.md` with the new documentation summary.
- Verification: documentation inspection only.

## 2026-06-23 - Agent Context Bootstrap

- Added `AGENTS.md` and `docs/` context files for future agents.
- Documented current declarations-only C++20 signaling-server state, stack, architecture boundaries, roadmap, verification limits, and documentation maintenance rules.
- Source: current repository inspection plus `session-ses_115d.md` planning context and `session-ses_114d.md` development context.
- Verification: source/documentation inspection only.

## 2026-06-21 - Initial Project Skeleton

- Created initial C++ project skeleton for a voice/video call signaling server.
- Added CMake project config with Boost, OpenSSL, and `nlohmann_json` dependency wiring.
- Added Dockerfile and `.dockerignore`.
- Added declarations for domain, application, and network layers under `include/buzzweb/`.
- Updated naming toward PascalCase methods/functions.
- Simplified `Participant` to identity/display name and changed `Room` participant storage to `std::vector<Participant>`.
- Local CMake configure previously failed because Boost package config was not available on the Windows host; Docker build was blocked because Docker Desktop's Linux engine was not running.
