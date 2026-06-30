# Build Process

## Prerequisites

- **Visual Studio 2022** with the following workloads:
  - .NET Desktop Development
  - Desktop development with C++ (Clang tools)
  - C++ Clang Compiler for Windows (clang-cl)
- **.NET Framework 4.8 SDK**
- **UPX** (Ultimate Packer for Executables) — optional, for compression
- **Code Signing Certificate** — optional, for Authenticode signing

## Solution Structure

The Visual Studio solution `Atomic Shield Client.sln` contains these projects:

| Project | Type | Language | Build Output |
|---------|------|----------|-------------|
| `AtomicAgent` | Windows Forms App | C# | `AtomicAgent.exe` |
| `EngineLoader` | Console App | C# | `EngineLoader.exe` |
| `RuntimeLoader` | Console App | C++ | `RuntimeLoader.exe` |
| `AtomicEngine` | Dynamic Library | C++ | `Atomic Engine.dll` |
| `Shared` | Static Library | C++ | `Shared.lib` |
| `SharedLoader` | Shared Project | C# | _(merged into consumers)_ |

### C# Projects (`AtomicAgent`, `EngineLoader`)

- Target: .NET Framework 4.8
- Platform: x64
- Uses NuGet packages:
  - `Microsoft.Web.WebView2` (AtomicAgent only)
  - `Microsoft.Win32.TaskScheduler` (AtomicAgent only)

### C++ Projects (`AtomicEngine`, `RuntimeLoader`, `Shared`)

- Platform: x64
- Toolset: ClangCL (`clang-cl`)
- Character set: Unicode
- Runtime Library: Multi-threaded (/MT for static linking)
- Language standard: C++17

**Include directories (AtomicEngine):**
```
$(SolutionDir)Shared\
$(SolutionDir)Shared/Helpers\
$(SolutionDir)Shared/Vendor\
$(SolutionDir)AtomicEngine/Vendor/BSThreadPool\
$(SolutionDir)AtomicEngine/Vendor/CRC32\
$(SolutionDir)AtomicEngine/Vendor/ixwebsocket\
$(SolutionDir)AtomicEngine/Vendor/libdatachannel\
$(SolutionDir)AtomicEngine/Vendor/xxHash\
```

## Build Steps

### 1. Visual Studio Build

Open `Atomic Shield Client.sln` in Visual Studio 2022 and build the solution (Build → Build Solution).

Or from command line:
```cmd
msbuild "Atomic Shield Client.sln" /p:Configuration=Release /p:Platform=x64
```

### 2. Post-Build Scripts (Optional)

After building, run the prepare scripts to compress and sign the binaries:

```cmd
Build\prepare.bat
Build\prepare-loader.bat
```

These scripts:
1. Copy built binaries to `Build\` directory
2. Compress with `UPX --best` (if UPX is available)
3. Sign with code signing certificate (if available)

### 3. Code Signing

`Build\sign.bat` signs `AtomicAgent.exe` using the SignTool:
```cmd
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256
    /v "path\to\AtomicAgent.exe"
```

*Note: The build scripts reference a code signing certificate by thumbprint. The actual certificate is not included in this repository.*

## Build Outputs

| File | Location | Description |
|------|----------|-------------|
| `AtomicAgent.exe` | `AtomicAgent\bin\Release\` | User-facing agent |
| `EngineLoader.exe` | `EngineLoader\bin\Release\` | C# loader |
| `RuntimeLoader.exe` | `RuntimeLoader\bin\Release\` | C++ loader (alternate) |
| `Atomic Engine.dll` | `AtomicEngine\bin\Release\` | Core anti-cheat engine |
| `Shared.lib` | `Shared\bin\Release\` | Static library |

After running prepare scripts, compressed and signed copies are placed in:
- `Build\`
- `Build\signed\`

## Configuration

### Version Number

Defined in `Shared/Common.h`:
```cpp
#define PRODUCT_VERSION_MAJOR 2
#define PRODUCT_VERSION_MINOR 2
#define PRODUCT_VERSION_BUILD 5
#define PRODUCT_VERSION_BETA  true
```

### API Endpoints

Defined in `Shared/Common.h`:
```cpp
// Default API URL — http://31.97.180.157:8002
```

### Logging

- AtomicAgent logs: `%APPDATA%\AtomicShield\AtomicAgent.log`
- AtomicEngine logs: `%LOCALAPPDATA%\AtomicShield\Trace.logs`

## Code Formatting

The repository includes `.clang-format` at the root level with Google-based rules:
- Allman brace placement
- 160-column column limit
- 4-space indentation (C++, no tabs)
- Aligned consecutive assignments
- Pointer/reference aligned to the right
