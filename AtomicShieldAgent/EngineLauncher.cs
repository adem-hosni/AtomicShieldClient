using AtomicAgent;
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

        public static int LoadEngineIntoLauncher(string enginePath, byte[] buffer)
        {
            Logger.AddDebugLog(nameof(LoadEngineIntoLauncher));

            // Manual mapping isn't trivial in C#, so we just emulate load-by-dump-and-start
            if (!DumpEngineProcess(enginePath, buffer))
                return -1;

            if (LaunchEngineProcess(enginePath, out var process) != LaunchResult.Success)
                return -2;

            return 0;
        }
    }
}
