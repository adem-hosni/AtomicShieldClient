# AtomicShield Client

> **Anti-Cheat System** — Version 2.2.5-beta

AtomicShield is a multi-layered anti-cheat solution. It combines a user-facing tray agent (C#), a native injection engine (C++), and a suite of runtime guards (memory scanning, process monitoring, anti-debugging) to detect and prevent cheating in real-time.

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                        AtomicAgent (C#)                          │
│  - System tray agent with WebView2 dashboard                     │
│  - Encrypted REST API client for auth/status                     │
│  - Downloads & launches EngineLoader via named pipe              │
└──────────────┬────────────────────────────────────────┬──────────┘
               │ named pipe                             │
               ▼                                        ▼
┌──────────────────────────────┐    ┌──────────────────────────────┐
│     EngineLoader (C#)        │    │   RuntimeLoader (C++)        │
│  - Named pipe server         │    │  - Named pipe server         │
│  - InProcessManualMapper     │    │  - SelfMapModule mapper      │
│  - Maps AtomicEngine DLL     │    │  - Maps AtomicEngine DLL     │
└──────────────┬───────────────┘    └──────────────┬───────────────┘
               │ manual map                        │ manual map
               └─────────────────┬─────────────────┘
                                 ▼
┌───────────────────────────────────────────────────────────────────┐
│                     AtomicEngine (C++ DLL)                        │
│  - WebSocket + HTTP encrypted network                             │
│  - 12 anti-debugging methods                                      │
│  - Process Guard (handle enumeration)                             │
│  - Heuristic Guard (xxHash memory scanning)                       │
│  - Manual Mapping Guard (PE header detection)                     │
│  - Crash reporting / Screenshot capture / HWID                    │
│  - Blacklisted driver scanning (~40 drivers)                      │
└───────────────────────────────────────────────────────────────────┘
```

## Components

| Component | Language | Description |
|-----------|----------|-------------|
| [`AtomicAgent/`](AtomicAgent/) | C# (.NET 8, WebView2) | System tray agent with WebView2 HTML/JS dashboard, encrypted API client, engine loader IPC |
| [`AtomicEngine/`](AtomicEngine/) | C++ DLL | Core anti-cheat engine — guards, anti-debugging, network, HWID, crash reports |
| [`EngineLoader/`](EngineLoader/) | C# Console | Named pipe server that manually maps the engine DLL into its own process |
| [`RuntimeLoader/`](RuntimeLoader/) | C++ EXE | Alternate native loader with named pipe + manual mapping |
| [`Shared/`](Shared/) | C++ Lib | Shared utilities: AES-256-CBC, Base64, logging, process detection |
| [`SharedLoader/`](SharedLoader/) | C# Shared | Shared logging class for both C# projects |

## Loading Chain

1. **AtomicAgent** starts (auto-start via Task Scheduler)
2. Agent pings the API server to verify connectivity
3. Agent downloads the encrypted engine DLL from the API and decrypts it
4. Agent writes `EngineLoader.exe` to `Program Files\AtomicShield\AtomicSvc.exe`
5. Agent launches the loader with UAC elevation (`runas`)
6. Agent opens a named pipe client (`\\.\pipe\AtomicPipe`) and sends the engine DLL bytes
7. **EngineLoader** receives the DLL, uses `InProcessManualMapper` to map it in-process
8. **AtomicEngine** `DllMain` executes: suspends threads, anti-debugging, network init, pulse loop
9. Engine connects via WebSocket to `ws://atomic-shield.com` and sends heartbeats

## Key Features

- **Anti-Debugging:** 12 detection methods (HW breakpoints, PEB flags, VEH, kernel debugger, process/window scanning, heap flags, debug port, closehandle exception)
- **Process Guard:** System handle enumeration — detects any process with an open handle to FiveM
- **Heuristic Guard:** xxHash-based memory region scanning for cheat signatures
- **Manual Mapping Guard:** PE header detection in non-standard memory regions
- **Blacklisted Drivers:** Scans for ~40 known malicious drivers
- **HWID Collection:** CPU, GPU (NVML/WMI), disks, SMBIOS, TPM (PowerShell), Steam ID
- **Encrypted Network:** AES-256-CBC with 8 rotating key/IV pairs
- **Screenshot Capture:** GDI+ capture of the FiveM window
- **Crash Reporting:** SEH handler → HTTPS POST to `atomic-shield.com/anticheat/crash-report`
- **Code Signing:** Authenticode + catalog signature verification via `WinVerifyTrust`
- **String Obfuscation:** skCrypter throughout C++ code

## Build Prerequisites

- Visual Studio 2022
- .NET 8 SDK
- C++ Clang tools (`clang-cl`)
- UPX (optional, for compression)
- Code signing certificate (optional)

## Building

See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions.

## Documentation

See the [docs/](docs/) directory:

- [Architecture](docs/ARCHITECTURE.md) — System architecture and data flow
- [AtomicAgent](docs/AtomicAgent.md) — C# agent documentation
- [AtomicEngine](docs/AtomicEngine.md) — C++ anti-cheat engine documentation
- [EngineLoader](docs/EngineLoader.md) — C# loader documentation
- [RuntimeLoader](docs/RuntimeLoader.md) — C++ loader documentation
- [Shared Library](docs/Shared.md) — Shared code documentation
- [Network Protocol](docs/NETWORK_PROTOCOL.md) — API and WebSocket protocol
- [Build Process](docs/BUILD.md) — Building from source
- [Security Features](docs/SECURITY.md) — Security mechanisms detail

## License

MIT — Copyright (c) 2024 Hyper

## Contact

- **Email:** hosniadem400@gmail.com & aifaouiameen@gmail.com

## Related Repositories

- [AtomicShield Server](https://github.com/adem-hosni/AtomicShieldServer) — Backend API and WebSocket server
- [AtomicShield Platform](https://github.com/adem-hosni/atomicshield-platform) — Management dashboard UI
