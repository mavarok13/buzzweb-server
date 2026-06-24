# Last Changes

Updated: 2026-06-24

## Current Documentation Update

- Clarified the project scope as a signaling-only server, not a WebRTC media server.
- Updated `docs/INFO.md`, `docs/STACK.md`, `docs/ARCHITECTURE.md`, and `docs/ROADMAP.md` to state that WebRTC media transport belongs to clients and external infrastructure.
- Documented that `buzzweb-server` should relay SDP offers, SDP answers, and ICE candidates as opaque JSON payloads, without parsing media, receiving RTP/RTCP, or acting as an SFU/MCU/media relay.

## Latest Code History Summary

- 2026-06-24: Clarified documentation scope: `buzzweb-server` is signaling-only and does not implement WebRTC media transport.
- 2026-06-24: Finalized and documented initial room-control JSON envelopes and examples for future implementation.
- 2026-06-24: Rewrote roadmap around repository/service contracts, typed results, participant lifecycle, signaling schemas, verification, and deployment steps.
- 2026-06-21: Created initial C++ project skeleton with CMake, Docker, and headers for domain, app, and net layers.
- 2026-06-21: Renamed method/function declarations to PascalCase style.
- 2026-06-21: Simplified `Participant` by removing join/connection state and changed `Room` participant storage from map-style planning to `std::vector<Participant>`.

Future agents should replace or extend this file with the newest concise summary after each change and append details to `docs/CHANGELOGS.md`.
