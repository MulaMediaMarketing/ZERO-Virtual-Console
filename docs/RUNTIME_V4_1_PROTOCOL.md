# ZERO Runtime V4.1 — Protocol V4

This slice replaces the legacy newline/tab SDK transport with a bounded framed protocol.

## Transport

Each message is encoded as:

- `uint32_le payload_length`
- payload body

The payload contains:

- `uint16 protocol_version`
- `uint16 message_type`
- `uint64 request_id`
- `uint32 field_count`
- repeated `uint32 field_length + UTF-8 bytes`

## Production invariants

- protocol version is currently `4`
- maximum frame payload is 64 KiB
- maximum field count is 16
- all fields must be valid UTF-8
- request/response correlation is mandatory
- authentication is the first message
- authentication tokens are generated only through `BCryptGenRandom`
- token generation fails closed; there is no time-based fallback
- malformed, oversized, or invalid frames terminate the connection

## Supported messages

- HELLO / WELCOME
- READY / ACK
- RESUME / ACK
- ACHIEVEMENT / ACK
- OVERLAY / OVERLAY_ACK
- PING / PONG
- ERROR

## Bootstrap

The SDK now consumes `runtime-v4.bootstrap` and requires bootstrap protocol version `4`.

## What this slice intentionally does not claim

Runtime V4.1 is not complete yet. The following hardening remains:

- create IPC/bootstrap before the game primary thread resumes
- READY deadline and explicit HandshakeFailure / ReadyTimeout outcomes
- heartbeat/watchdog and hung-game handling
- reconnect policy
- secure named-pipe ACL review
- asynchronous SDK receive queue so overlay events cannot interleave with synchronous request responses
- minidump capture and richer crash diagnostics
- SQLite platform persistence

This document describes the framed transport slice only.
