# BuzzWeb — Server
**BuzzWeb** — a self-hosted C++ voice/video room app for private communication.

The server manages rooms, participants, and WebSocket-based signaling.  
Media is transmitted directly between clients using WebRTC; it does not pass
through this server.

## Main features
- **No accounts** — users only enter a display name and create or join a room.
- **Room-based communication** — users join rooms using a short room code.
- **Optional room password** — rooms may be protected with a password.
- **Self-hosted** — the server is intended to run on your own machine or VPS.

## Tech stack
- C++20
- CMake
- Boost.Asio
- Boost.Beast
- Boost.Log
- nlohmann/json
- WebSocket signaling
- WebRTC media on the client side

## Installation and running

### Docker (recommended)

Requirements: Git and a running Docker engine.

```bash
git clone https://github.com/mavarok13/buzzweb-server.git
cd buzzweb-server
docker build -t buzzweb-server .
docker run --rm -p 9291:9291 \
  -e BUZZWEB_TLS_ENABLED=0 \
  buzzweb-server
```

The plain WebSocket endpoint is available at `ws://localhost:9291`.

To run the development image with WSS, use Bash, Git Bash, or WSL:

```bash
./scripts/docker-build.sh
./scripts/generate-dev-certs.sh localhost 127.0.0.1
./scripts/docker-run-dev.sh
```

The WSS endpoint is available at `wss://localhost:9291`. If `mkcert` is not
installed, the certificate script falls back to OpenSSL and the generated
self-signed certificate must be trusted manually.

### Local build

Requirements:

- CMake 3.24 or newer
- A C++20 compiler
- Boost with the System, Log, LogSetup, and Thread components
- OpenSSL
- nlohmann/json

On Debian 12, install the build dependencies with:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake libboost-all-dev libssl-dev ninja-build \
  nlohmann-json3-dev pkg-config
```

Configure and build the executable:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

For a local plain WebSocket server:

```bash
BUZZWEB_TLS_ENABLED=0 ./build/buzzweb_server
```

PowerShell equivalent:

```powershell
$env:BUZZWEB_TLS_ENABLED = "0"
.\build\buzzweb_server.exe
```

TLS is enabled by default. To run the local executable with development
certificates:

```bash
./scripts/generate-dev-certs.sh localhost 127.0.0.1
BUZZWEB_CERTIFICATE_FILE_PATH=certs/dev-cert.pem \
BUZZWEB_PRIVATE_KEY_PATH=certs/dev-key.pem \
./build/buzzweb_server
```

### Runtime configuration

| Variable | Required | Description |
| --- | --- | --- |
| `BUZZWEB_TLS_ENABLED` | No | TLS is enabled by default. `0`, `false`, or `FALSE` selects plain WS. |
| `BUZZWEB_CERTIFICATE_FILE_PATH` | With TLS | Path to the PEM certificate file. |
| `BUZZWEB_PRIVATE_KEY_PATH` | With TLS | Path to the PEM private key file. |
| `BUZZWEB_SERVER_PORT` | No | Listen port; defaults to `9291`. |

The standalone test client is available at `client_demo/index.html`. Open it in
two browser tabs and connect both tabs to the same WS or WSS endpoint for manual
room and WebRTC signaling checks.

## Contributing

1. Read `AGENTS.md` and the context documents under `docs/` before changing code.
2. Create a focused branch from the branch selected for the change.
3. Follow the existing C++20 style and keep WebRTC media outside the server.
4. Update the relevant guidance documents. Every completed change must update
   `docs/LAST_CHANGES.md` and add the newest entry to `docs/CHANGELOGS.md`.
5. Build the project and run applicable checks. If local dependencies prevent a
   full build, describe the limitation and the checks that did run.
6. Keep commits scoped and open a pull request with the motivation, behavior
   changes, and verification results.

Bug reports and feature proposals should include reproduction steps or a clear
use case, expected behavior, and relevant logs or protocol messages. Never
commit generated builds, private keys, certificates, or other secrets.
