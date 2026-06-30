# AtomicAgent

**Location:** `AtomicAgent/`  
**Language:** C# (.NET 8, WebView2)  
**Role:** User-facing system tray agent — manages the anti-cheat lifecycle, provides an HTML/JS dashboard via WebView2, communicates with the backend API, and orchestrates engine loading.

## Files

| File | Purpose |
|------|---------|
| `Main.cs` | Application entry point, STA thread init, API health check, dashboard launch |
| `AtomicAPI.cs` | Encrypted REST API client for auth, status, engine download |
| `ServerEndPoint.cs` | TCP latency measurement for server selection |
| `EngineLauncher.cs` | Dumps loader to disk, launches elevated, sends engine via named pipe |
| `AtomicEncoder.cs` | AES-256-CBC encryption/decryption with 8 rotating key/IV pairs |
| `Buffer.cs` | Embedded shellcode buffers and hardcoded PE binary data |
| `DashboardForm.cs` | WebView2-based dashboard with custom window dragging and controls |
| `LatencyEvaluator.cs` | Server selection logic with registry-based latency caching |
| `Logger.cs` | Thread-safe file logging to `%APPDATA%\AtomicShield\AtomicAgent.log` |
| `StartupManager.cs` | Task Scheduler integration for auto-start on login |
| `WinAPI.cs` | P/Invoke declarations for memory management and process creation |

## Initialization Flow (`Main.cs`)

1. Application context created, form hidden on startup
2. STA thread launched for clipboard/COM compatibility
3. API health check ping sent to `http://31.97.180.157:8002`
4. Latency evaluator loads cached server data from registry
5. Dashboard (`DashboardForm`) launched via WebView2
6. EngineLauncher begins the engine loading sequence

## API Client (`AtomicAPI.cs`)

- **Endpoint:** `http://31.97.180.157:8002`
- All requests use **AES-256-CBC encryption** via `AtomicEncoder`
- Requests are byte-arrays sent as HTTP POST bodies
- Communication flow:
  1. Serialize request object to JSON
  2. Encrypt with a randomly selected key/IV index
  3. Prepend the key index byte to the ciphertext
  4. Send as raw POST body
  5. Receive response, decrypt using same key index
  6. Deserialize JSON response

## Encryption (`AtomicEncoder.cs`)

- **Algorithm:** AES-256-CBC with PKCS7 padding
- **Keys:** 8 pre-defined key/IV pairs stored as byte arrays
- **Key selection:** Random index (0-7) chosen per-request, appended as first byte of payload
- Also provides MD5 hashing utility

## Engine Launcher (`EngineLauncher.cs`)

1. Extracts embedded `EngineLoader.exe` from resources to `C:\Program Files\AtomicShield\AtomicSvc.exe`
2. Launches the loader with `runas` verb (UAC elevation)
3. Waits for loader pipe connection
4. Reads engine DLL from `Buffer.AtomicSvcProcessBuffer` (hardcoded byte array)
5. Sends base64-encoded DLL over named pipe `\\.\pipe\AtomicPipe`
6. Sends `load_code:` prefix command

## Dashboard (`DashboardForm.cs`)

- WebView2-based dashboard UI
- Custom window dragging via `WM_NCHITTEST` override
- Minimize to tray, close to tray (hidden background operation)
- `enableShield` button triggers `EngineLauncher` reload

## Startup Manager (`StartupManager.cs`)

- Registers/unregisters the agent in Windows Task Scheduler
- Creates a task that runs on user logon with highest privileges

## Embedded Buffers (`Buffer.cs`)

Contains static byte arrays:
- **Shellcode buffers** — Obfuscated loader shellcode (raw x86/x64 bytes)
- **`AtomicSvcProcessBuffer`** — A hardcoded PE binary (the encrypted/runtime engine DLL)

## Dependencies

- .NET 8
- `Microsoft.Web.WebView2` — WebView2 control
- `System.IO.Pipes` — Named pipe IPC
- `TaskScheduler` — Auto-start registration
