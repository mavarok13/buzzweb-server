# Info

`buzzweb-server` is a C++20 WebSocket/WSS signaling server for a self-hosted voice/video calling app. It manages rooms, participants, and signaling messages; it does not implement WebRTC media transport. Voice/video media belongs to clients and WebRTC infrastructure outside this server.

## Current Project State

- The repository now has initial domain, application, and minimal WSS networking implementations under `src/`.
- There are public headers under `include/buzzweb/` for domain, application, and network layers.
- There is an executable target for running the WSS server with environment-based TLS/port configuration; there are no tests yet.
- The networking layer can own an IO context/TLS context, accept TCP connections, perform TLS and WebSocket handshakes, parse text JSON control envelopes, dispatch to `ControlDispatcher`, write JSON responses, send structured room participant events, and relay WebRTC offer/answer/ICE messages through `SessionRegistry`.
- CMake currently defines a compiled static library target named `buzzweb_server_lib`, alias `BuzzWeb::Server`, and executable target `buzzweb_server`.
- Dockerfile exists as a dependency-contained build environment for configuring and building the executable image.
- `client_demo/index.html` is a standalone browser test client for two-tab room signaling and real peer-to-peer WebRTC audio/video using the server only as a signaling relay.

## Intended MVP Features

- Create a temporary call room with a short room code.
- Join a room by room code.
- Support optional room password hashes.
- Track participants in a room.
- Exchange WebRTC signaling messages through WebSocket/WSS as opaque JSON payloads: offers, answers, and ICE candidates.
- Keep voice/video media outside the server; the first version should use client-to-client WebRTC media transport.
- Deploy the signaling server on a self-hosted machine or VPS.

## Current Classes

- `domain::Participant`: participant identity and display name.
- `domain::Room`: room code, optional password hash, and vector of participants.
- `domain::RoomRepository`: storage abstraction for rooms, including participant membership checks and conditional empty-room cleanup.
- `domain::InMemoryRoomRepository`: thread-safe in-memory room storage using one repository-level mutex, transactional update copies, and conditional empty-room removal.
- `app::RoomService`: implemented use-case layer for creating, joining, leaving, listing room participants, and checking room membership.
- `app::ControlDispatcher`: implemented control-message router for create, join, leave, and WebRTC signaling relay messages using the current application structs, plus structured callbacks for participant events and targeted relay delivery.
- `net::Server`: top-level WSS server owner for IO context, TLS context, listener, and session registry.
- `net::Listener`: TCP accept loop that creates sessions.
- `net::Session`: per-connection TLS/WebSocket owner that reads text frames, dispatches JSON control messages, and writes JSON responses.
- `net::SessionRegistry`: thread-safe active-session registry keyed by participant ID.

## Current Limitations

- The executable starts the WSS server using `BUZZWEB_CERTIFICATE_FILE_PATH`, `BUZZWEB_PRIVATE_KEY_PATH`, and optional `BUZZWEB_SERVER_PORT`.
- Initial room-control and WebRTC signaling JSON envelopes and examples are finalized in `AGENTS.md`; dispatcher-level routing and network parser/serializer implementations exist for direct create/join/leave and offer/answer/ICE responses.
- `participant_joined` and `participant_left` event delivery now uses structured event data: joined events include a participant object plus the current participants list, and left events include the departed participant ID plus the remaining participants list.
- No WebRTC media stack is implemented or planned inside this server; SDP and ICE data is relayed as opaque JSON and not parsed as media.
- No authentication, authorization, TURN integration, or persistence exists yet.
- The browser demo uses a public Google STUN server and manual endpoint/participant input; production clients will need configurable ICE servers and stronger identity/session handling.
- No automated tests are configured.
- Local CMake verification currently depends on installing/configuring Boost, OpenSSL, and `nlohmann_json`.
