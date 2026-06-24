# Last Changes

Updated: 2026-06-24

## Current Documentation Update

- Rewrote `docs/ROADMAP.md` to reflect the current implementation priorities.
- Added roadmap items for `RoomRepository::Update()` contract decisions, typed `RoomService` results, participant identity lifecycle, dispatcher-to-service mapping, and result-to-JSON error mapping.
- Kept the existing direction: declarations-first C++20 WSS signaling server, WebRTC media outside the server, `RoomService` as the mutation boundary, and in-memory rooms as the MVP storage model.

## Latest Code History Summary

- 2026-06-24: Rewrote roadmap around repository/service contracts, typed results, participant lifecycle, signaling schemas, verification, and deployment steps.
- 2026-06-21: Created initial C++ project skeleton with CMake, Docker, and headers for domain, app, and net layers.
- 2026-06-21: Renamed method/function declarations to PascalCase style.
- 2026-06-21: Simplified `Participant` by removing join/connection state and changed `Room` participant storage from map-style planning to `std::vector<Participant>`.

Future agents should replace or extend this file with the newest concise summary after each change and append details to `docs/CHANGELOGS.md`.
