# EngineLoader

**Location:** `EngineLoader/`  
**Language:** C# (.NET 8, Native AOT)  
**Role:** Named pipe server that receives the AtomicEngine DLL from AtomicAgent and maps it into its own process using manual PE loading.

## Files

| File | Purpose |
|------|---------|
| `Program.cs` | Entry point — creates and starts the pipe server |
| `PipeServer.cs` | Named pipe server (`\\.\pipe\AtomicPipe`) that receives and processes DLL bytes |
| `InProcessManualMapper.cs` | Full PE manual mapper — resolves imports, relocations, TLS, SEH |
| `PEImage.cs` | PE structure definitions (DOS header, NT headers, sections, directories) |

## Flow

```
EngineLoader.exe launched (elevated)
└── Program.Main()
    └── PipeServer.Start()
        ├── Create named pipe: \\.\pipe\AtomicPipe
        ├── Wait for client connection
        ├── Read message from pipe
        │   ├── Contains prefix + base64-encoded DLL
        │   └── Prefix: "load_code:" + base64 data
        ├── Decode base64 → byte[] (PE image)
        └── InProcessManualMapper.Map(bytes)
            ├── Validate DOS/PE headers
            ├── Allocate memory for image
            ├── Copy sections (.text, .rdata, .data, .reloc)
            ├── Resolve relocations (IMAGE_BASE_RELOCATION)
            ├── Resolve imports (IAT)
            ├── Register SEH handlers (if any)
            ├── Execute TLS callbacks
            └── Call DllMain (DLL_PROCESS_ATTACH)
```

## Pipe Server (`PipeServer.cs`)

- Creates `\\.\pipe\AtomicPipe` as a named pipe server
- Uses `NamedPipeServerStream` with `PipeDirection.InOut`
- Single-threaded — processes one client at a time
- Protocol:
  - Client sends: `load_code:` prefix followed by base64-encoded DLL bytes
  - Server decodes and passes to `InProcessManualMapper`
- On completion, pipe server exits (AtomicEngine is now running in-process)

## Manual Mapper (`InProcessManualMapper.cs`)

A complete PE loader that maps a DLL without using `LoadLibrary`. This avoids detection by LoadLibrary hooks and module enumerations.

### Mapping Steps

1. **Header Validation** — Checks DOS magic (`MZ`) and NT signature (`PE`)
2. **Memory Allocation** — `VirtualAlloc` at preferred base address (or any address), `PAGE_EXECUTE_READWRITE`
3. **Section Copying** — Copies each section from the PE file to the allocated memory
4. **Relocation Resolution** — Iterates `IMAGE_BASE_RELOCATION` blocks and applies delta fixups
5. **Import Resolution** — For each imported DLL:
   - Calls `LoadLibrary` for the DLL
   - Resolves each function via `GetProcAddress`
   - Writes function addresses into the IAT
6. **Exception Handler Registration** — Calls `RtlAddFunctionTable` for x64 exception handlers
7. **TLS Callbacks** — Executes TLS callback array before DllMain
8. **DllMain Call** — Calls the DLL entry point with `DLL_PROCESS_ATTACH`

## PE Structures (`PEImage.cs`)

Defines the managed interop structures needed for PE parsing:

- `IMAGE_DOS_HEADER` — DOS header with `e_magic` and `e_lfanew`
- `IMAGE_NT_HEADERS` — NT headers signature + file/optional headers
- `IMAGE_FILE_HEADER` — Machine, sections count, characteristics
- `IMAGE_OPTIONAL_HEADER64` — Image base, entry point, section alignment, size
- `IMAGE_SECTION_HEADER` — Section name, virtual address, raw size/pointer
- `IMAGE_DATA_DIRECTORY` — Data directory entries
- `IMAGE_BASE_RELOCATION` — Relocation block structure
- `IMAGE_IMPORT_DESCRIPTOR` — Import descriptor structure
- `IMAGE_TLS_DIRECTORY` — TLS directory for callback execution
