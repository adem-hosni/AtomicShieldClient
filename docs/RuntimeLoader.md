# RuntimeLoader

**Location:** `RuntimeLoader/`  
**Language:** C++ (Console EXE)  
**Role:** Alternate native loader that receives the AtomicEngine DLL via named pipe and maps it in-process using manual PE loading. Serves as a C++ alternative to the C# EngineLoader.

## Files

| File | Purpose |
|------|---------|
| `Main.cpp` | Entry point — runs the pipe server |
| `RuntimeLoader.cpp/.h` | Native manual PE mapper |
| `CPipeServer.cpp/.h` | Named pipe server (`\\.\pipe\AtomicRuntime`) |
| `XorStr.h` | String obfuscation utility |

## Flow

```
RuntimeLoader.exe launched
└── Main()
    └── CPipeServer::Start()
        ├── Create named pipe: \\.\pipe\AtomicRuntime
        ├── Wait for client connection
        ├── Read message from pipe
        │   ├── Prefix: "load_code:" + base64-encoded DLL
        │   └── Decode base64
        └── RuntimeLoader::SelfMapModule()
            ├── Allocate memory
            ├── Copy PE headers + sections
            ├── Resolve relocations
            ├── Resolve imports
            ├── TLS callbacks
            └── Call DllMain
```

## Named Pipe Server (`CPipeServer`)

- Creates `\\.\pipe\AtomicRuntime` (note: different pipe name vs C# loader's `AtomicPipe`)
- `CreateNamedPipeA` with `PIPE_ACCESS_DUPLEX`
- Blocks on `ConnectNamedPipe` waiting for client
- Reads message, checks for `load_code:` prefix
- Decodes base64-encoded payload
- Passes raw bytes to `RuntimeLoader::SelfMapModule`

## Native Manual Mapper (`RuntimeLoader`)

- **Memory:** `VirtualAlloc` with `PAGE_EXECUTE_READWRITE`
- **Sections:** Copies each PE section from source
- **Relocations:** Iterates `IMAGE_BASE_RELOCATION` blocks, applies delta
- **Imports:** `LoadLibraryA` + `GetProcAddress` for each import entry
- **TLS:** Finds and executes TLS callback table
- **Entry:** Calls `DllMain` via `CreateThread` + `WaitForSingleObject`

## Differences from C# EngineLoader

| Aspect | EngineLoader (C#) | RuntimeLoader (C++) |
|--------|-------------------|---------------------|
| Language | C# (.NET) | C++ (native) |
| Pipe name | `\\.\pipe\AtomicPipe` | `\\.\pipe\AtomicRuntime` |
| Entry call | Direct function pointer | `CreateThread` + wait |
| Exception handling | `RtlAddFunctionTable` | Not implemented |
| Dependencies | .NET 8 (Native AOT) | None (pure Win32) |
| String obfuscation | N/A | `XorStr.h` / skCrypter |

## Dependencies

- Win32 API (kernel32, ntdll)
- `Shared/Vendor/skCrypter.h` — String obfuscation
