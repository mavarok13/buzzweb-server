# Info

`buzzweb-server` is a C++20 server project for a self-hosted voice/video calling app. The current focus is the WebSocket/WSS control and signaling server for rooms and participants; media is planned to use WebRTC between clients.

## Current Project State

- The repository is currently a declarations-only skeleton.
- There are public headers under `include/buzzweb/` for domain, application, and network layers.
- There are no `.cpp` implementations, no `src/` directory, no executable target, and no tests yet.
- CMake currently defines an `INTERFACE` library target named `buzzweb_server` and alias `BuzzWeb::Server`.
- Dockerfile exists as a dependency-contained build environment, but the current project has no compiled sources.

## Intended MVP Features

- Create a temporary call room with a short room code.
- Join a room by room code.
- Support optional room password hashes.
- Track participants in a room.
- Exchange WebRTC signaling messages through WebSocket/WSS: offers, answers, and ICE candidates.
- Keep voice/video media outside the server, using WebRTC client-to-client transport for the first version.
- Deploy the signaling server on a self-hosted machine or VPS.

## Current Classes

- `domain::Participant`: participant identity and display name.
- `domain::Room`: room code, optional password hash, and vector of participants.
- `domain::RoomRepository`: storage abstraction for rooms.
- `app::RoomService`: intended use-case layer for creating, joining, leaving, and listing room participants.
- `app::ControlDispatcher`: intended JSON control-message router between network sessions and `RoomService`.
- `net::Server`: intended top-level WSS server owner.
- `net::Listener`: intended TCP/TLS accept loop owner.
- `net::Session`: intended per-WebSocket connection owner.
- `net::SessionRegistry`: intended active-session registry keyed by participant ID.

## Current Limitations

- No methods are implemented.
- No executable starts the server.
- No HTTP/WebSocket runtime behavior exists yet.
- No protocol message schema is finalized beyond planning examples.
- No authentication, authorization, TURN integration, persistence, or room cleanup exists yet.
- No automated tests are configured.
- Local CMake verification currently depends on installing/configuring Boost, OpenSSL, and `nlohmann_json`.
