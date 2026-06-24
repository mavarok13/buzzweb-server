# Changelogs

Keep newest entries first. Append an entry for every completed change, including documentation-only changes.

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
