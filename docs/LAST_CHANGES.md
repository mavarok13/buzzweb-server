# Last Changes

Updated: 2026-07-12

## Current Implementation Update

- Participant event delivery now uses structured event data. `participant_joined` includes the joined participant object and current participants list; `participant_left` includes the departed participant ID and remaining participants list.
- Runtime event delivery recipients come from the already prepared participant list rather than rereading the room after leave/disconnect.
- `main.cpp` no longer rereads rooms after leave/disconnect just to build event recipients.
- `RoomService::LeaveAllRooms()` swallows only expected `room_not_found` and `not_in_room` cleanup misses; unexpected errors still propagate.
- `Session::RemoveFromRegistry()` logs cleanup exceptions and keeps the session close path from being broken by cleanup failures.

## Latest Code History Summary

- 2026-07-12: Added structured participant event data and tightened leave/disconnect cleanup behavior.
- 2026-07-11: Added standalone browser WebRTC signaling demo under `client_demo/`.
- 2026-07-11: Added conditional empty-room cleanup and updated repository/runtime guidance.
- 2026-07-11: Added WebRTC offer/answer/ICE signaling relay with target-session availability checks.
- 2026-07-08: Added executable/runtime event wiring and verified Docker build after compile fixes.
- 2026-07-08: Added dev Docker/certificate helper scripts.
- 2026-07-08: Fixed Boost.Beast SSL websocket build failure and verified Docker image build.
- 2026-06-26: Added minimal Boost.Asio/Beast WSS networking implementation and CMake wiring.
- 2026-06-25: Added initial domain/app source implementations and static library CMake wiring.
- 2026-06-25: Documented the thread-safe repository and transactional `Update()` contract decision in agent guidance and architecture docs.
- 2026-06-24: Clarified documentation scope: `buzzweb-server` is signaling-only and does not implement WebRTC media transport.
- 2026-06-24: Finalized and documented initial room-control JSON envelopes and examples for future implementation.
- 2026-06-24: Rewrote roadmap around repository/service contracts, typed results, participant lifecycle, signaling schemas, verification, and deployment steps.
- 2026-06-21: Created initial C++ project skeleton with CMake, Docker, and headers for domain, app, and net layers.
- 2026-06-21: Renamed method/function declarations to PascalCase style.
- 2026-06-21: Simplified `Participant` by removing join/connection state and changed `Room` participant storage from map-style planning to `std::vector<Participant>`.

Future agents should replace or extend this file with the newest concise summary after each change and append details to `docs/CHANGELOGS.md`.
