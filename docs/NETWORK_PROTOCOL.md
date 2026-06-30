# Network Protocol

## Overview

AtomicShield uses two network communication channels:

1. **HTTP/HTTPS REST API** — AtomicAgent ↔ API server (auth, status, engine download)
2. **WebSocket** — AtomicEngine ↔ WebSocket server (real-time heartbeat, events, commands)

All communication is encrypted with AES-256-CBC using 8 rotating key/IV pairs.

## API Server

**Base URL:** `http://31.97.180.157:8002`

### Authentication / Request Format

Every API request between AtomicAgent and the server follows this format:

```
Request:
  1. Serialize JSON payload
  2. Encrypt with AES-256-CBC (random key index 0-7)
  3. Prepend key index byte to ciphertext
  4. Send as HTTP POST body (raw bytes)

Response:
  1. Read key index from first byte
  2. Decrypt remaining bytes with corresponding key/IV
  3. Deserialize JSON
```

### Endpoints

| Endpoint | Method | Direction | Purpose |
|----------|--------|-----------|---------|
| `/health` / ping | GET | Agent → Server | Connectivity check |
| API (undocumented) | POST | Agent → Server | Authentication, status reporting, engine download |

*Note: The specific API endpoints beyond health check are called through the encrypted payload mechanism — the endpoint paths themselves are not visible in the client code as they are constructed dynamically.*

## WebSocket Protocol

**Server:** `ws://atomic-shield.com`

### Connection

1. AtomicEngine connects to the WebSocket server on initialization
2. Connection is persistent (reconnects on disconnect)
3. Heartbeat sent approximately every 1 second

### Packet Format

```
[Encrypted Payload]
  ├── Key Index (1 byte) — selects which AES key/IV to use
  └── Ciphertext (variable) — AES-256-CBC encrypted binary data
```

All packets use the same AES-256-CBC encryption scheme with 8 rotating key/IV pairs.

### Message Types

| Type | Direction | Description |
|------|-----------|-------------|
| Heartbeat | Engine → Server | Status flags, uptime, guard state |
| HWID | Engine → Server | Hardware identifiers on connect |
| Detection Event | Engine → Server | Triggered when a guard detects cheating |
| Screenshot | Engine → Server | JPEG screenshot of FiveM window |
| Commands | Server → Engine | Reload, update configuration, etc. |

### Heartbeat Payload

Sent every pulse (1 second), contains:
- Engine status (running, guarding, scanning)
- Uptime in seconds
- Guard states (ProcessGuard, HeuristicGuard, ManualMapGuard — enabled/disabled/alerts)
- Anti-debugging status
- Detection count

### Detection Event

When any guard triggers:
1. Engine immediately sends detection event
2. Includes: guard type, description, severity
3. Server may respond with action command (kick, ban, etc.)

### Engine Reload Command

When the server sends a reload command:
1. Engine receives the command via WebSocket
2. Triggers `SelfMapModule` to load a new engine instance
3. Old instance continues until new one is ready
4. Seamless transition with no service interruption

## Encryption Details

### AES-256-CBC Parameters

- **Algorithm:** AES-256 in CBC mode
- **Key Size:** 32 bytes (256 bits)
- **IV Size:** 16 bytes (128 bits)
- **Padding:** PKCS7
- **Key/IV Pairs:** 8 pre-defined sets

### Key/IV Storage

Keys are stored as hardcoded byte arrays in:
- `AtomicAgent/AtomicEncoder.cs` (C#)
- `Shared/CAtomicCore.cpp` (C++)

Both sides share identical key/IV sets for interoperability.

### Key Selection

Each message selects a key index (0-7):
- **Random selection** — AtomicAgent randomly picks for each API request
- **Fixed per-message** — The index is prepended as a 1-byte prefix to the ciphertext

This provides key rotation without requiring a separate key exchange protocol.

## Crash Report Upload

**Endpoint:** `https://atomic-shield.com/anticheat/crash-report`

**Method:** HTTP POST (multipart/form-data)

**Fields:**
- `exception_code` — Windows exception code
- `exception_address` — Address where exception occurred
- `registers` — CPU register state at crash
- `engine_version` — Product version string
- `timestamp` — Crash time

**File Attachments:**
- Screenshot (JPEG) — FiveM window capture at crash time
- Trace logs (`Trace.logs`) — From `%LOCALOCALAPPDATA%\AtomicShield\`

## Network Security Summary

| Aspect | Implementation |
|--------|---------------|
| Encryption | AES-256-CBC with rotating keys |
| Key storage | Hardcoded in client binary (obfuscated) |
| Transport | HTTP / WebSocket (no TLS observed in client) |
| Crash data | HTTPS POST to crash endpoint |
| IP collection | Network interface enumeration by engine |
