# Stack

Technical stack for `buzzweb-server`.

## Language And Project Config

- Language: C++20.
- Build system: CMake, minimum version 3.24.
- Current library target: `buzzweb_server_lib` as a compiled static library.
- Current alias target: `BuzzWeb::Server`.
- Current executable target: `buzzweb_server`.
- Current stage: domain, application, minimal WSS networking, and executable runtime wiring have `.cpp` implementations.

The executable loads TLS certificate/private-key paths and port from environment variables.

## C++ Dependencies

- Boost.System for Boost.Asio and Boost.Beast networking support.
- Boost.Beast for WebSocket handling. Beast is header-only inside Boost.
- Boost.Asio for networking and TLS stream integration.
- Boost.Log and Boost.LogSetup for logging.
- Boost.Thread for Boost.Log/threading support.
- OpenSSL for TLS/WSS and security primitives.
- `nlohmann_json` for JSON control/signaling messages.

Planned or possible later dependencies:

- `coturn` for external STUN/TURN infrastructure used by clients.
- `libdatachannel` or `libwebrtc` only for native client-side WebRTC work if this repository's scope later grows beyond the signaling server.
- PostgreSQL, Redis, or SQLite only if persistent rooms/session state becomes necessary.
- GoogleTest or Catch2 when tests are added.

## Runtime Services

- WebSocket or WSS signaling endpoint served by this C++ server.
- WebRTC media transport between clients, outside this server. Media should not flow through WebSocket in the MVP.
- STUN/TURN service, likely `coturn`, for client NAT traversal once real WebRTC clients are tested.
- Optional reverse proxy such as Caddy or nginx for TLS termination, depending on deployment design.

## Runtime Configuration

Runtime configuration is loaded from environment variables by the executable.

The library-level `net::ServerConfig` currently carries the listen port, TLS certificate file, and TLS private-key file.

Current environment variables:

- `BUZZWEB_CERTIFICATE_FILE_PATH` for the TLS certificate file.
- `BUZZWEB_PRIVATE_KEY_PATH` for the TLS private-key file.
- `BUZZWEB_SERVER_PORT` as an optional listen port override.

Likely future settings:

- Listen address.
- Listen port.
- TLS certificate file.
- TLS private key file.
- Log level.
- Allowed origins or deployment host settings.
- TURN/STUN URLs advertised to clients.
- Optional room/session timeouts.

## Deployment

- `Dockerfile` uses `debian:12-slim` and installs `build-essential`, `cmake`, `ninja-build`, Boost, OpenSSL, `nlohmann_json`, and `pkg-config`.
- Current Docker build command runs CMake configure and builds the `buzzweb_server` executable image.
- No `docker-compose.yml` exists yet.
- A realistic deployment will likely need the signaling server container plus external `coturn`, and optionally a reverse proxy.

Likely future ports:

- `443` or `8443` TCP for HTTPS/WSS signaling.
- `3478` UDP/TCP for STUN/TURN.
- `5349` TCP/TLS for TURN over TLS.
- A restricted TURN relay UDP range for small deployments.

## Tooling Notes

- `.opencode/` contains local assistant/tooling configuration, including `codebase-index` support.
- `.claude/cache/codebase-index/` contains local index cache data.
- `build/` contains generated CMake files from previous local configure attempts and is ignored by git.
- No CI workflow is currently configured.
- No test framework is currently configured.

## Local Verification Notes

- Previous local CMake configure failed because Boost package config was unavailable on this Windows host.
- Previous Docker verification failed because Docker Desktop's Linux engine was not running.
- Agents may run CMake or Docker verification when dependencies/services are already available, but should not install dependencies or start services without user approval.
