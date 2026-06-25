# Last Changes

Updated: 2026-06-25

## Current Documentation Update

- Added agent guidance for thread-safe `RoomRepository` implementations.
- Recorded the MVP decision that `InMemoryRoomRepository` should use one repository-level mutex, while `RoomRepository::Update()` should use a transactional copy-then-commit workflow.
- Clarified that `RoomRepository::Remove()` must be synchronized with `Update()` and that `Room` should remain a mutex-free domain value object for the MVP.

## Latest Code History Summary

- 2026-06-25: Documented the thread-safe repository and transactional `Update()` contract decision in agent guidance and architecture docs.
- 2026-06-24: Clarified documentation scope: `buzzweb-server` is signaling-only and does not implement WebRTC media transport.
- 2026-06-24: Finalized and documented initial room-control JSON envelopes and examples for future implementation.
- 2026-06-24: Rewrote roadmap around repository/service contracts, typed results, participant lifecycle, signaling schemas, verification, and deployment steps.
- 2026-06-21: Created initial C++ project skeleton with CMake, Docker, and headers for domain, app, and net layers.
- 2026-06-21: Renamed method/function declarations to PascalCase style.
- 2026-06-21: Simplified `Participant` by removing join/connection state and changed `Room` participant storage from map-style planning to `std::vector<Participant>`.

Future agents should replace or extend this file with the newest concise summary after each change and append details to `docs/CHANGELOGS.md`.
