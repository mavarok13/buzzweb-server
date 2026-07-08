# Last Changes

Updated: 2026-07-08

## Current Implementation Update

- Added dev helper scripts under `scripts/` for generating local certs, building the Docker image, and running the Docker container with mounted certs.
- Fixed Docker build compilation for SSL WebSocket sessions by including Boost.Beast's SSL websocket support header in `Session.hpp`.
- Docker image build now completes successfully with `docker build -t buzzweb-server .`.
- Executable/config wiring exists, but broadcasts, empty-room cleanup, and WebRTC offer/answer/ICE relay are still not implemented.

## Latest Code History Summary

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
