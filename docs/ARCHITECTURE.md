# Architecture

## System Overview

AtomicShield is a multi-component anti-cheat system designed for FiveM game servers. It operates as a layered defense platform with a user-facing agent, a native injection engine, and multiple runtime detection guards.

```
┌─────────────────────────────────────────────────────────────────────┐
│                         User Context                                 │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐    │
│  │                    AtomicAgent (C# WinForms)                  │    │
│  │  ┌─────────────┐  ┌───────────┐  ┌───────────────────────┐  │    │
│  │  │ Dashboard    │  │ AtomicAPI │  │ EngineLauncher        │  │    │
│  │  │ (WebView2)   │  │ (REST)    │  │ (Pipe IPC to Loader)  │  │    │
│  │  └─────────────┘  └───────────┘  └───────────┬───────────┘  │    │
│  └───────────────────────────────────────────────┼──────────────┘    │
│                                                   │ named pipe       │
│  ┌───────────────────────────────────────────────┼──────────────┐    │
│  │              EngineLoader (C# Console)         │              │    │
│  │  ┌──────────────────┐  ┌───────────────────┐  │              │    │
│  │  │ PipeServer        │  │ InProcessManualMap. │  │              │    │
│  │  │ (\\.\pipe\Atomic...)│  │ (PE Mapper)       │  │              │    │
│  │  └──────────────────┘  └────────┬──────────┘  │              │    │
│  └───────────────────────────────────┼────────────┘              │    │
│                                      │ manual map                │    │
└──────────────────────────────────────┼───────────────────────────┘    │
                                       ▼                                │
┌──────────────────────────────────────────────────────────────────────┐│
│                  FiveM Process / EngineLoader Process                 ││
│                                                                      ││
│  ┌─────────────────────────────────────────────────────────────┐     ││
│  │                AtomicEngine (C++ DLL)                        │     ││
│  │                                                              │     ││
│  │  ┌─────────────┐  ┌──────────┐  ┌───────────────────────┐  │     ││
│  │  │ CAntiDebug   │  │ CAtomic  │  │ CGuardManager          │  │     ││
│  │  │ (12 methods) │  │ Network  │  │ ┌─────────────────┐   │  │     ││
│  │  └─────────────┘  │ (WS+HTTP) │  │ │ CProcessGuard   │   │  │     ││
│  │                   └──────────┘  │ │ CHeuristicGuard  │   │  │     ││
│  │  ┌─────────────┐  ┌──────────┐  │ │ CManualMapGuard │   │  │     ││
│  │  │ CAtomicHWID │  │ Screensh.│  │ └─────────────────┘   │  │     ││
│  │  └─────────────┘  └──────────┘  └───────────────────────┘  │     ││
│  │  ┌─────────────┐  ┌──────────┐                              │     ││
│  │  │ CCrashHandler│  │ BasicChks│                              │     ││
│  │  └─────────────┘  └──────────┘                              │     ││
│  └─────────────────────────────────────────────────────────────┘     ││
└──────────────────────────────────────────────────────────────────────┘
```

## Loading Sequence

```
Time  AtomicAgent                  EngineLoader                  AtomicEngine
 │       │                              │                              │
 │       │ 1. Check API health           │                              │
 │       │ 2. Download encrypted DLL     │                              │
 │       │ 3. Decrypt DLL                │                              │
 │       │ 4. Write EngineLoader.exe     │                              │
 │       ├──────────────────────────────►│                              │
 │       │ 5. Launch (elevated)          │                              │
 │       │ 6. Open named pipe            │                              │
 │       │ 7. Send DLL bytes over pipe   │                              │
 │       │ 8. Send "load_code:" msg      │                              │
 │       │                               │ 9. Receive DLL bytes         │
 │       │                               │10. Parse PE headers          │
 │       │                               │11. ManualMap DLL             │
 │       │                               ├──────────────────────────────►│
 │       │                               │12. DllMain (DLL_PROCESS_ATTACH)│
 │       │                               │    - Suspend existing threads │
 │       │                               │    - CAtomicAntiCheat::Init  │
 │       │                               │    - CAntiDebugging::Start   │
 │       │                               │    - CAtomicNetwork::Start   │
 │       │                               │    - CGuardManager::Start    │
 │       │                               │    - Heartbeat pulse loop    │
 │       │                               │                              │
 │       │                               │13. WebSocket connect         │
 │       │                               │◄── ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─►│
 │       │                               │    Encrypted packet exchange │
```

## Component Communication

| From | To | Method | Protocol |
|------|----|--------|----------|
| AtomicAgent | API Server | HTTPS | REST (JSON, AES-256-CBC encrypted) |
| AtomicAgent | EngineLoader | Named Pipe | `\\.\pipe\AtomicPipe` — base64-encoded raw bytes |
| AtomicEngine | WebSocket Server | WSS | Binary packets (AES-256-CBC) |
| AtomicEngine | API Server | HTTPS | Crash reports (WinHTTP, multipart form) |

## Memory Layout

```
EngineLoader.exe Process
┌──────────────────────────────────┐
│  EngineLoader.exe (host)         │
│  ┌────────────────────────────┐  │
│  │ AtomicEngine.dll (manual   │  │
│  │ mapped, no LoadLibrary)    │  │
│  │ - .text (executable)       │  │
│  │ - .rdata                   │  │
│  │ - .data                    │  │
│  │ - .reloc                   │  │
│  │ - TLS callbacks            │  │
│  └────────────────────────────┘  │
│  Heap (engine allocations)       │
│  Threads (NtCreateThreadEx)      │
└──────────────────────────────────┘
```

## Security Layers

1. **Transport Security:** AES-256-CBC with 8 rotating key/IV pairs
2. **Process Obfuscation:** Manual mapping (no `LoadLibrary`), `NtCreateThreadEx` with hidden-from-debugger flags
3. **String Obfuscation:** skCrypter encrypts all string literals at compile time
4. **Anti-Debugging:** 12 methods covering user-mode and kernel-mode detection
5. **Runtime Guards:** Process handle scanning, memory signature scanning, PE header detection
6. **Crash Handling:** SEH handler captures and uploads crash dumps
7. **Code Signing Verification:** Authenticode and catalog signatures checked
8. **Integrity Checks:** Secure Boot, HVCI, test signing mode verification

## Network Architecture

```
┌─────────────┐     HTTPS (AES)     ┌──────────────────┐
│ AtomicAgent │◄───────────────────►│  API Server       │
│             │                     │  31.97.180.157:   │
│             │                     │  8002             │
└─────────────┘                     └──────────────────┘
                                                   

┌──────────────┐   WSS (AES packets)   ┌──────────────────┐
│ AtomicEngine │◄─────────────────────►│  WebSocket Server │
│              │                       │  ws://atomic-     │
│              │                       │  shield.com       │
└──────────────┘                       └──────────────────┘
                                                   
┌──────────────┐   HTTPS (multipart)  ┌──────────────────┐
│ AtomicEngine │─────────────────────►│  Crash Report API │
│ (CCrashHndlr)│                       │  atomic-shield.  │
│              │                       │  com/anticheat/   │
│              │                       │  crash-report     │
└──────────────┘                       └──────────────────┘
```
