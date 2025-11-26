using AtomicAgent;
using MongoDB.Bson.Serialization.Serializers;
using System;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Windows.Media.Protection.PlayReady;

namespace AtomicShield
{
    public static class EngineLauncher
    {
        private static NamedPipeClientStream _ClientPipe;
        private static StreamReader _reader;
        private static StreamWriter _writer;

        public enum LaunchResult
        {
            Success,
            LaunchElevationFailed
        }

        public static string GetEnginePath()
        {
            try
            {
                string programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
                string basePath = Path.Combine(programFiles, "AtomicShield");

                if (!Directory.Exists(basePath))
                {
                    Directory.CreateDirectory(basePath);
                    Logger.AddDebugLog("Created AtomicShield directory in Program Files");
                }

                return Path.Combine(basePath, "AtomicSvc.exe");
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog($"Failed to resolve Program Files path: {ex.Message}");
                return Path.Combine(Directory.GetCurrentDirectory(), "AtomicSvc.exe");
            }
        }

        public static bool DumpEngineProcess(string enginePath, byte[] buffer)
        {
            try
            {
                string dir = Path.GetDirectoryName(enginePath)!;
                Directory.CreateDirectory(dir);

                File.WriteAllBytes(enginePath, buffer);
                Logger.AddDebugLog($"Engine dumped to {enginePath} ({buffer.Length})");
                return true;
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog($"Failed to dump engine: {ex.Message}");
                return false;
            }
        }

        public static LaunchResult LaunchEngineProcess(string enginePath, out Process? process)
        {
            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = enginePath,
                    UseShellExecute = true,
                    Verb = "runas",
                    WorkingDirectory = Path.GetDirectoryName(enginePath)
                };

                process = Process.Start(psi);
                Logger.AddDebugLog("Engine launched successfully");
                return LaunchResult.Success;
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog($"Launch failed: {ex.Message}");
                process = null;
                return LaunchResult.LaunchElevationFailed;
            }
        }

        public static async Task ConnectToPipeServer()
        {
            _ClientPipe = new NamedPipeClientStream(".", "AtomicPipe", PipeDirection.InOut);
            Logger.AddDebugLog("Connecting to pipe server..");
            await _ClientPipe.ConnectAsync();
            Logger.AddDebugLog("Successfuly connected to pipe server");

            _reader = new StreamReader(_ClientPipe, Encoding.UTF8);
            _writer = new StreamWriter(_ClientPipe, Encoding.UTF8) { AutoFlush = true };
        }

        public static async Task<bool> LoadEngineIntoLauncher(byte[] buffer)
        {
            Logger.AddDebugLog(nameof(LoadEngineIntoLauncher));

            await ConnectToPipeServer();

            // Encode buffer to Base64
            string base64Buffer = Convert.ToBase64String(buffer);
            await _writer.WriteLineAsync("load_code:" + base64Buffer);

            Task.Delay(700).Wait();

            string response = await _reader.ReadLineAsync();
            _ClientPipe.Close();
            if (response.StartsWith("load_success:"))
            {
                Logger.AddDebugLog("Engine loaded successfully into launcher");
                return true;
            }
            else
            {
                Logger.AddDebugLog("Failed to load engine into launcher: " + response);
                return false;
            }
        }

        public static async void LoadEngine(AtomicAPI atomicApi)
        {
            byte[] engineBuffer = await atomicApi.DownloadEngineAsync();
            string encodedLoaderBuffer = await atomicApi.DownloadClientLoader();

            if (engineBuffer == null || engineBuffer.Length == 0)
            {
                Logger.AddDebugLog("Failed to download engine buffer.");
                return;
            }

            if (string.IsNullOrEmpty(encodedLoaderBuffer))
            {
                Logger.AddDebugLog("Failed to download loader buffer.");
                return;
            }

            byte[] loaderBuffer = Convert.FromBase64String(encodedLoaderBuffer);

            Logger.AddDebugLog("Engine downloaded successfully. Size: {0} bytes", engineBuffer.Length);
            string enginePath = GetEnginePath();

            // Close AtomicSvc.exe if it's running
            if (Process.GetProcessesByName("AtomicSvc").Length > 0)
            {
                foreach (var proc in Process.GetProcessesByName("AtomicSvc"))
                {
                    try
                    {
                        proc.Kill();
                        proc.WaitForExit();
                        Logger.AddDebugLog("Closed existing AtomicSvc.exe process (PID: {0})", proc.Id);
                    }
                    catch (Exception ex)
                    {
                        Logger.AddDebugLog("Failed to close AtomicSvc.exe process (PID: {0}): {1}", proc.Id, ex.Message);
                    }
                }
            }

            if (DumpEngineProcess(enginePath, loaderBuffer))
            {
                Logger.AddDebugLog("Engine process dumped successfully.");
                Process atomicService;
                LaunchResult LaunchResult = LaunchEngineProcess(enginePath, out atomicService);
                if (LaunchResult == EngineLauncher.LaunchResult.Success)
                {
                    await LoadEngineIntoLauncher(engineBuffer);
                }
                else
                {
                    Logger.AddDebugLog("Failed to launch engine process. {0}", LaunchResult);
                }
            }
            else
            {
                Logger.AddDebugLog("Failed to dump engine process.");
            }
        }

    }

}
