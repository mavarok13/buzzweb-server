# BuzzWeb — Server
**BuzzWeb** — a self-hosted C++ voice/video room app for private communication.

The server manages rooms, participants, and WebSocket-based signaling.  
Media is planned to be transmitted directly between clients using WebRTC.

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

## TO DO
- make enum of message, result and error types for requests and responses
- move `ErrorMessage` in `ControlDispatcher.cpp` strings to another file
- refactor repository