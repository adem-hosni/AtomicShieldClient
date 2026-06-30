# Documentation Index

## Files

| Document | Description |
|----------|-------------|
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | System architecture, component relationships, loading sequence, memory layout, network topology |
| [`AtomicAgent.md`](AtomicAgent.md) | C# system tray agent — API client, encryption, dashboard, engine launcher, startup |
| [`AtomicEngine.md`](AtomicEngine.md) | C++ anti-cheat engine DLL — initialization, anti-debugging, guards, network, HWID, crash handler |
| [`EngineLoader.md`](EngineLoader.md) | C# named-pipe loader — pipe server, manual PE mapping (sections, relocations, IAT, TLS, SEH) |
| [`RuntimeLoader.md`](RuntimeLoader.md) | C++ native loader — alternative to EngineLoader with native pipe server and mapper |
| [`Shared.md`](Shared.md) | Shared C++ library — AES-256-CBC crypto, Base64, logging, process detection, runtime import resolver |
| [`NETWORK_PROTOCOL.md`](NETWORK_PROTOCOL.md) | Network communication — REST API format, WebSocket protocol, packet encryption, heartbeat, crash reports |
| [`BUILD.md`](BUILD.md) | Build process — prerequisites, solution structure, build steps, signing, configuration |
| [`SECURITY.md`](SECURITY.md) | Security features — all 9 defense layers with implementation details and detection methods |
