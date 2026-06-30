# Shared Library

**Location:** `Shared/`  
**Language:** C++ (Static Library)  
**Role:** Shared utilities, cryptography, and common code used by both the C++ engine components.

## Files

| File | Purpose |
|------|---------|
| `CAtomicCore.cpp/.h` | AES-256-CBC encryption with 8 key/IV sets, MD5 hashing |
| `SharedUtil.cpp/.h` | Debug logging, Base64, process/PID detection, token privs, registry, random strings |
| `RuntimeImportResolver.h` | Dynamic import resolution via `GetProcAddress` with obfuscated strings |
| `SecurityChecks.cpp/.h` | **Stubbed/disabled** — anti-breakpoint, anti-debug CPUID, anti-dump |
| `SharedProtocols.h` | **Stub** — process mitigation policies |
| `SharedChecks.h` | **Stub** — malicious process scanning |
| `CLatencyEvaluator.h/.cpp` | Server latency evaluation |
| `CServerEndPoint.h/.cpp` | HTTP-based server latency measurement |
| `Common.h` | Version defines, API URLs, build timestamp |

### Helpers (`Shared/Helpers/`)

| File | Purpose |
|------|---------|
| `AES.h/.cpp` | AES-256 CBC encryption implementation |

### Vendor (`Shared/Vendor/`)

| File | Purpose |
|------|---------|
| `skCrypter.h` | Compile-time string encryption |
| `XorStr.h` | XOR-based compile-time string obfuscation |

## Crypto (`CAtomicCore`)

- **Algorithm:** AES-256-CBC with PKCS7 padding
- **Keys:** 8 pre-defined 32-byte keys and 16-byte IVs
- Same key/IV sets as `AtomicEncoder.cs` (C# side) — ensures cross-compatibility
- **MD5:** Simple MD5 hash implementation for checksumming

## Debug Logging (`SharedUtil::AddDebugLog`)

- Writes to `%LOCALAPPDATA%\AtomicShield\Trace.logs`
- Thread-safe via `std::mutex`
- Timestamped log entries
- Used extensively throughout AtomicEngine for diagnostics

## Utility Functions (`SharedUtil`)

| Function | Purpose |
|----------|---------|
| `AddDebugLog` | Write timestamped log entry |
| `Base64Encode` / `Base64Decode` | Base64 encoding/decoding |
| `GetProcessByName` | Find process ID by executable name |
| `GetProcessByWindowClass` | Find process ID by window class |
| `FindFiveM` | Locate FiveM process via window class `grcWindow` |
| `SetDebugPrivilege` | Enable `SeDebugPrivilege` |
| `RegistryRead` / `RegistryWrite` | Read/write registry values |
| `RandomString` | Generate cryptographically random strings |
| `GetKnownFolderPath` | Resolve CSIDL/FOLDERID paths |

## Dynamic Import Resolution (`RuntimeImportResolver`)

- Resolves API functions at runtime using `GetModuleHandleA` + `GetProcAddress`
- All function/DLL names use `skCrypt` (skCrypter) for string obfuscation
- Provides wrappers for: `VirtualAllocEx`, `VirtualProtectEx`, `WriteProcessMemory`, `ReadProcessMemory`, registry APIs, and more
- Avoids static import table entries that could be hooked or monitored

## Stubbed Features

The following files exist but contain commented-out or stub implementations:

- **SecurityChecks** — Anti-breakpoint, anti-debug CPUID, anti-dump (all disabled)
- **SharedProtocols** — Process mitigation policy application (not implemented)
- **SharedChecks** — Malicious process scanning (not implemented)

## Version Info (`Common.h`)

```cpp
#define PRODUCT_VERSION_MAJOR 2
#define PRODUCT_VERSION_MINOR 2
#define PRODUCT_VERSION_BUILD 5
#define PRODUCT_VERSION_BETA  true
```

Build timestamp auto-generated via `__DATE__` / `__TIME__`.
