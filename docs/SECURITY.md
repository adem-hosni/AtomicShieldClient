# Security Features

## Overview

AtomicShield implements multiple layers of security to detect, prevent, and report cheating in FiveM game sessions. This document details each security mechanism.

## Layer 1: Anti-Debugging

**12 detection methods** in `CAntiDebugging` — covers both user-mode and kernel-mode debugger detection.

| # | Method | What it detects | Implementation |
|---|--------|-----------------|----------------|
| 1 | Hardware Breakpoints (`Dr0-Dr3`) | Debuggers setting HW breakpoints | Reads `CONTEXT` via `GetThreadContext` or `NtGetContextThread` |
| 2 | PEB `BeingDebugged` | Basic `IsDebuggerPresent` | Reads `PEB.BeingDebugged` byte directly |
| 3 | PEB `NtGlobalFlag` | Debugger environment | Checks `PEB.NtGlobalFlag` for `FLG_HEAP_ENABLE_TAIL_CHECK` etc. |
| 4 | `NtQueryInformationProcess` | Process debug port | Queries `ProcessDebugPort` (returns -1 if debugged) |
| 5 | CloseHandle Exception | Debugger exception handling | `CloseHandle(INVALID_HANDLE)` — raises `STATUS_INVALID_HANDLE` only if debugger present |
| 6 | Heap Flags | Debug heap allocation | Checks `HEAP_GROWABLE` flag absence (debuggers set `HEAP_TAIL_CHECKING` etc.) |
| 7 | Kernel Debugger | KDNET / WinDbg kernel debug | `NtQuerySystemInformation(SystemKernelDebuggerInformation)` |
| 8 | `DbgBreakPoint` Patching | Prevents debugger thread init | Patches `DbgBreakPoint` in ntdll to `ret` |
| 9 | `DbgUiRemoteBreakin` Patching | Prevents remote breakin | Patches `DbgUiRemoteBreakin` in ntdll to `ret` |
| 10 | Process Window Scan | Debugger GUI windows | Scans for window class names matching known debuggers |
| 11 | Process Name Scan | Debugger processes | Enumerates running processes for known debugger executable names |
| 12 | VEH Detection | Vectored Exception Handler | Checks for handlers installed by debuggers |

## Layer 2: System Integrity Checks

**`BasicChecks`** verifies the system's security posture.

### Secure Boot
- **Method:** WMI query `SELECT IsSecureBootEnabled FROM Win32_ComputerSystem`
- **Purpose:** Ensures system boot chain is trusted
- **Action:** Report if disabled (many cheats require Secure Boot off)

### Test Signing Detection (6 methods)
1. `g_CiEnabled` — Reads Code Integrity enabled flag from `ci.dll`
2. `g_CiOptions` — Reads CI options from `ci.dll`
3. `SeCiCallbacks` — Scans for CI callback pointers
4. `ElamDrv` registry — Checks Early Launch Anti-Malware driver registry
5. `CodeIntegrityOptions` — Reads kernel CI options
6. `g_CipInteger` — Additional CI flag in `ci.dll`
- **Purpose:** Detects test-signing mode (required for loading unsigned cheat drivers)

### HVCI / Memory Integrity
- **Method:** WMI query `SELECT IsRunning FROM Win32_DeviceGuard WHERE ... VirtualizationBasedSecurity` + registry check
- **Purpose:** Ensures Hypervisor-protected Code Integrity is enabled
- **Action:** Report if disabled

### Blacklisted Driver Scanning
- **Method:** `EnumDeviceDrivers` + driver name matching against ~40 known malicious drivers
- **Coverage:** Game cheat drivers, vulnerable drivers used for privilege escalation
- **Action:** Report blacklisted driver to server

## Layer 3: Runtime Guards

### Process Guard (`CProcessGuard`)
- **Technique:** `NtQuerySystemInformation(SystemHandleInformation)` — enumerates all system handles
- **Detection:** Any process holding an open handle to the FiveM process
- **Rationale:** Cheat injection tools open `OpenProcess` on the game to read/write memory or inject code
- **Allowlist:** Known-safe processes excluded

### Heuristic Guard (`CHeuristicGuard`)
- **Technique:** xxHash-based memory region scanning
- **Process:** Scans memory regions of running processes, computes xxHash of each region, compares against known cheat signature database
- **Scope:** All processes, not just FiveM
- **Configurable:** Scan interval, region size limits, signature updates from server

### Manual Mapping Guard (`CManualMappingGuard`)
- **Technique:** Memory region enumeration + PE header detection
- **Detection:** `MZ` signature in memory regions not backed by a loaded module
- **Purpose:** Catches manually-mapped DLLs (common anti-cheat evasion technique where DLLs are mapped without `LoadLibrary`)

### Plugin Scanning
- **Technique:** Enumerates FiveM plugin directories for unauthorized plugin files
- **Purpose:** Detects cheat plugins injected into FiveM's plugin system

## Layer 4: Code Integrity

### Authenticode Verification (`FileAuthentication`)
- **Method:** `WinVerifyTrust` with `DRIVER_ACTION_VERIFY` / `HTTPSPROV_ACTION`
- **Purpose:** Verifies embedded digital signatures on executable files
- **Scope:** Checks both Authenticode signatures and catalog-signed files

### Code Signing in Build
- **Tool:** SignTool with SHA256 digest
- **Timestamp:** DigiCert timestamp server
- **Output:** All release binaries are signed during build process

## Layer 5: Process Obfuscation

### Manual Mapping (No LoadLibrary)
- EngineLoader and RuntimeLoader both use manual PE mappers
- DLL is loaded without `LoadLibrary` → invisible to `CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)` and `LdrLoadDll` hooks
- Only detectable via memory region scanning (which the ManualMappingGuard itself does)

### Stealth Threads (`CAtomicThread`)
- **API:** `NtCreateThreadEx` with `THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER`
- **Effect:** Threads hidden from debugger, not suspended by process freeze

### Self-Map Module Reload
- Engine can reload itself in-process without creating new processes
- Uses manual mapping to load fresh instance while old instance runs

## Layer 6: String Obfuscation

### skCrypter (`Shared/Vendor/skCrypter.h`)
- Compile-time string literal encryption
- Each string is XOR-encrypted with a random key at compile time
- Decrypted at runtime just before use
- Applied to all sensitive strings: API URLs, function names, registry paths, pipe names, class names

### XorStr (`Shared/Vendor/XorStr.h`, `RuntimeLoader/XorStr.h`)
- Simple compile-time XOR string obfuscation
- Used in RuntimeLoader for basic string protection

## Layer 7: Encryption

### AES-256-CBC
- All network communication encrypted
- 8 rotating key/IV pairs prevent key reuse analysis
- Random key selection per-message
- Same implementation in C# and C++ for cross-compatibility

## Layer 8: Crash Handling

### SEH Handler (`CCrashHandler`)
- Captures all unhandled exceptions via `SetUnhandledExceptionFilter`
- Collects: exception code, address, CPU registers, callstack
- Captures screenshot of FiveM window at crash moment
- Uploads via HTTPS multipart POST to crash endpoint
- Prevents cheat developers from analyzing engine crashes locally

## Layer 9: HWID Collection

| Method | Data | Purpose |
|--------|------|---------|
| WMI | CPU ID, GPU, disks, SMBIOS UUID | Hardware fingerprinting |
| NVML | NVIDIA GPU UUID | Additional GPU identification |
| PowerShell | TPM manufacturer/version | Hardware root of trust identification |
| Registry | Steam ID | User account binding |

## Security Notes

- **Code signing credentials** are exposed in `Build/*.bat` scripts — these should be kept private for public releases
- **AES keys** are hardcoded in both C# and C++ binaries (obfuscated by skCrypter in C++, plain byte arrays in C#) — determined attacker could extract them
- **No TLS** is used in the observed client-server communication (encryption is application-layer only)
- **Stubbed features** (`SecurityChecks`, `SharedProtocols`, `SharedChecks`, `CModuleGuard`) indicate planned but incomplete security features
