# AtomicEngine

**Location:** `AtomicEngine/`  
**Language:** C++ (DLL)  
**Role:** Core anti-cheat engine — injected into the FiveM game process to detect cheating via multiple runtime guards, anti-debugging, memory scanning, and encrypted network communication.

## Files

| File | Purpose |
|------|---------|
| `dllmain.cpp` | DLL entry point — thread suspension, init sequence, pulse loop |
| `CAtomicAntiCheat.cpp/.h` | Main orchestrator — lifecycle, detection handling, hard kick |
| `CAtomicNetwork.cpp/.h` | WebSocket client via ixwebsocket, packet protocol, heartbeat, IP chain |
| `CAntiDebugging.cpp/.h` | 12-method anti-debugging system |
| `BasicChecks.cpp/.h` | Plugin scanning, Secure Boot, test signing, HVCI, blacklisted drivers |
| `CAtomicHWID.cpp/.h` | Hardware ID collection (CPU, GPU, disks, SMBIOS, TPM, Steam) |
| `CAtomicHook.cpp/.h` | 5-byte JMP detour + trampoline function hooking |
| `CAtomicThread.cpp/.h` | `NtCreateThreadEx` wrapper for stealth thread creation |
| `CGuardManager.cpp/.h` | Manages all runtime guard instances |
| `CCrashHandler.cpp/.h` | SEH handler + WinHTTP crash report upload |
| `Screenshot.cpp/.h` | GDI+ screenshot capture of FiveM window |
| `SelfMapModule.cpp/.h` | In-process manual PE mapper for engine reload |
| `FileAuthentication.cpp/.h` | Authenticode + catalog signature verification |
| `Utils.cpp/.h` | CRC32, process enum, Steam ID, module maps, function hook detection |
| `Structs.h` | Internal PEB/LDR structure definitions |
| `StdInc.h` | Master include / precompiled header |

### Guards (`AtomicEngine/Guards/`)

| File | Purpose |
|------|---------|
| `CGuardBase.h/.cpp` | Abstract guard interface (Init, Update, Stop) |
| `CHeuristicGuard.h/.cpp` | xxHash-based memory region scanning for cheat signatures |
| `CProcessGuard.h/.cpp` | System handle enumeration — detects open handles to FiveM |
| `CManualMappingGuard.h/.cpp` | PE header detection in non-standard memory regions |
| `CModuleGuard.h` | **Disabled** — was to hook `LdrLoadDll` |

## Initialization Sequence (`dllmain.cpp`)

```
DLL_PROCESS_ATTACH
├── Suspend all existing threads (except current)
├── Set process dpiawareness
├── CAtomicAntiCheat::Init()
│   ├── CAtomicNetwork::Start() → WebSocket connect
│   ├── CAntiDebugging::Start() → 12 detection methods
│   ├── BasicChecks::Perform()
│   ├── CGuardManager::Start()
│   │   ├── CProcessGuard::Init()
│   │   ├── CHeuristicGuard::Init()
│   │   └── CManualMappingGuard::Init()
│   └── CAtomicHWID::Collect()
├── CAtomicAntiCheat::StartPulse()
│   └── Every 1 second (pulse loop):
│       ├── CAntiDebugging::OverrideCheck()
│       ├── CGuardManager::Update()
│       ├── CAtomicNetwork::Pulse()
│       └── CAtomicAntiCheat::OnPulse()
└── CAtomicAntiCheat::Start() → signals ready
```

## Anti-Debugging (`CAntiDebugging`)

12 detection methods, run on init and on each pulse:

1. **`CheckHardwareBreakpoints`** — Scans `CONTEXT.Dr0-Dr3` for active HW breakpoints
2. **`CheckBeingDebugged`** — Checks PEB `BeingDebugged` flag
3. **`CheckNtGlobalFlag`** — Checks PEB `NtGlobalFlag` (set by debugger)
4. **`CheckNtQueryInformationProcess`** — Uses `NtQueryInformationProcess(ProcessDebugPort)`
5. **`CheckCloseHandleException`** — Triggers `CloseHandle` on invalid handle, catches `STATUS_INVALID_HANDLE` if debugger present
6. **`CheckHeapFlags`** — Checks `HEAP_GROWABLE` flag absence (debugger sets extra flags)
7. **`CheckKernelDebugger`** — Calls `NtQuerySystemInformation(SystemKernelDebuggerInformation)`
8. **`CheckDbgBreakPoint`** — Patches `DbgBreakPoint` to `ret` to prevent debugger init
9. **`CheckDbgUiRemoteBreakin`** — Patches `DbgUiRemoteBreakin` to `ret`
10. **`CheckProcessWindow`** — Scans for debugger process windows by class name
11. **`CheckProcessByName`** — Scans running processes for known debugger names
12. **`CheckVEH`** — Detects vectored exception handlers installed by debuggers

## Basic Checks (`BasicChecks`)

- **Plugin Scanning** — Enumerates FiveM plugin directories for unauthorized plugins
- **Secure Boot** — Checks `IsSecureBootEnabled` via WMI
- **Test Signing (6 methods):**
  - `g_CiEnabled` / `g_CiOptions` in `ci.dll`
  - `SeCiCallbacks` detection
  - `ElamDrv` registry key check
  - `CodeIntegrityOptions` in kernel
  - `g_CipInteger` in `ci.dll`
  - Binary search for test-signed driver patterns
- **HVCI / Memory Integrity** — WMI query + registry check
- **Blacklisted Drivers** — Scans loaded kernel modules (`\Device\PhysicalMemory` or `EnumDeviceDrivers`) against ~40 known cheat/vulnerable driver names

## Network (`CAtomicNetwork`)

- **WebSocket** via `ixwebsocket` library
- **Server:** `ws://atomic-shield.com` (or configured endpoint)
- **Encryption:** AES-256-CBC via `CAtomicCore` (Shared lib) — same 8 key/IV pairs as C# side
- **Protocol:** Binary packets with encrypted payload
- **Message flow:**
  - Connect on engine start
  - Send heartbeat every pulse (1s)
  - Send HWID on connect
  - Send detection events when triggered
  - Receive commands (e.g., engine reload via `SelfMapModule`)
- **Heartbeat:** Contains status flags, uptime, guard state
- **IP Chain Collection** — Traces network interfaces and routes

## Guards

### Process Guard (`CProcessGuard`)
- Uses `NtQuerySystemInformation(SystemHandleInformation)` to enumerate system handles
- Filters for handles with `FiveM.exe` PID as target
- Detects any process with an open handle to FiveM (cheat process injection)
- Maintains allowlist for known-safe processes

### Heuristic Guard (`CHeuristicGuard`)
- Scans memory regions of all running processes for known cheat signatures
- Uses xxHash to compute memory region hashes and compares against known-bad hash database
- Configurable scan interval and region size limits

### Manual Mapping Guard (`CManualMappingGuard`)
- Enumerates memory regions in the FiveM process
- Checks for PE signatures (`MZ` header) in regions not backed by a valid module
- Flags manually-mapped DLLs (common cheat injection technique)

## Crash Handler (`CCrashHandler`)

- Sets `SetUnhandledExceptionFilter` as SEH handler
- On crash:
  1. Captures exception context (registers, callstack)
  2. Takes screenshot of FiveM window
  3. Collects `Trace.logs` from `%LOCALAPPDATA%\AtomicShield\`
  4. Uploads via WinHTTP `POST` to `https://atomic-shield.com/anticheat/crash-report`
  5. Multipart form data with text fields + file attachments

## HWID Collection (`CAtomicHWID`)

| Method | Data Collected |
|--------|---------------|
| WMI (Win32_Processor) | CPU ID, name, manufacturer |
| WMI (Win32_VideoController) | GPU name, manufacturer |
| NVML (NVIDIA) | GPU UUID via NVML library (if NVIDIA GPU) |
| WMI (Win32_DiskDrive) | Disk model, serial number, size |
| WMI (Win32_ComputerSystem) | Manufacturer, model, SMBIOS UUID |
| PowerShell (`Get-Tpm`) | TPM manufacturer ID, version |
| Registry | Steam ID (`HKEY_CURRENT_USER\Software\Valve\Steam\ActiveProcess\ActiveUser`) |

## Screenshot (`Screenshot`)

- Uses GDI+ (`GdiplusStartup`, `Graphics.CopyFromScreen`)
- Locates FiveM window by class name (`grcWindow`)
- Captures client area bitmap
- JPEG-compressed for upload

## SelfMapModule (`SelfMapModule`)

- In-process manual PE mapper used for engine reload
- Maps a new DLL instance without `LoadLibrary`
- Resolution: sections, relocations, IAT, TLS, SEH handlers
- Used by `CAtomicNetwork` when receiving a "reload" command from server
- Thread-safe: suspends guard threads during remap

## Thread Management (`CAtomicThread`)

- Wraps `NtCreateThreadEx` with `THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER`
- Bypasses process freeze/suspend (threads hidden from `CreateToolhelp32Snapshot`)
- Used for all engine worker threads

## Function Hooking (`CAtomicHook`)

- 5-byte JMP detour (relative near jump)
- Writes `E9 <relative_offset>` at target function start
- Allocates trampoline with original bytes + JMP back
- Saves original bytes for unhooking

## Dependencies

- **ixwebsocket** — WebSocket client (`AtomicEngine/Vendor/ixwebsocket/`)
- **xxHash** — Fast hashing (`AtomicEngine/Vendor/xxHash/`)
- **BSThreadPool** — Thread pool (`AtomicEngine/Vendor/BSThreadPool/`)
- **libdatachannel** — WebRTC data channel (`AtomicEngine/Vendor/libdatachannel/`)
- **CRC32** — CRC32 checksum (`AtomicEngine/Vendor/CRC32/`)
- **skCrypter** — String obfuscation (`Shared/Vendor/skCrypter.h`)
- **Shared** — AES crypto, utilities, logging static library
