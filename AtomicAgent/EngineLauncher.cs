using AtomicAgent;
using AtomicShield;
using AtomicAgent.ManualMapper;
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace AtomicShield
{
    public static class EngineLauncher
    {
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
                Logger.AddDebugLog($"Engine dumped to {enginePath}");
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
                    CreateNoWindow = true,
                    UseShellExecute = false
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

        public static int LoadEngineIntoLauncher(nint TargetProcess, string enginePath, byte[] buffer)
        {
            Logger.AddDebugLog(nameof(LoadEngineIntoLauncher));

            ManualMapper.ManualMapModule(TargetProcess, buffer);

            return 0;
        }

        public static async void LoadEngine(AtomicAPI atomicApi)
        {
            string engineBuffer = await atomicApi.DownloadEngineAsync();

            if (!string.IsNullOrEmpty(engineBuffer))
            {
                Logger.AddDebugLog("Engine downloaded successfully. Size: {0} bytes", engineBuffer.Length);
                string enginePath = EngineLauncher.GetEnginePath();

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

                if (EngineLauncher.DumpEngineProcess(enginePath, AtomicAgent.Buffer.AtomicSvcProcessBuffer))
                {
                    Logger.AddDebugLog("Engine process dumped successfully.");
                    Process atomicService;
                    EngineLauncher.LaunchResult LaunchResult = EngineLauncher.LaunchEngineProcess(enginePath, out atomicService);
                    if (LaunchResult == EngineLauncher.LaunchResult.Success)
                    {
                        EngineLauncher.LoadEngineIntoLauncher(atomicService.Handle, enginePath, System.Text.Encoding.UTF8.GetBytes(engineBuffer));
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
            else
            {
                Logger.AddDebugLog("Failed to download engine.");
            }
        }

    }
}
