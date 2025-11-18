using AtomicShield;
using System;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AtomicAgent
{
    internal class ApiCheckResult
    {
        public bool Success { get; set; } = true;
        public bool Initialized { get; set; } = false;
        public string Title { get; set; } = "Loading Content Manifest...";
        public string Message { get; set; } = "The agent is loading. This won't take long.";
        public System.Text.Json.JsonDocument Status { get; set; }
    }

    internal static class Program
    {
        static AtomicAPI _atomicApi;

        static async void LoadEngine()
        {
            string engineBuffer = await _atomicApi.DownloadEngineAsync();

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

                if (EngineLauncher.DumpEngineProcess(enginePath, Buffer.AtomicSvcProcessBuffer))
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

        [STAThread]
        private static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            string cmdLine = Environment.CommandLine;
            Logger.AddDebugLog("AtomicShield Agent started with command line: {0}", cmdLine);

            DeleteOldAgentIfExists(cmdLine);

            _atomicApi = new AtomicAPI(new AtomicEncoder());
            ApiCheckResult apiResult = RunApiChecksSync(_atomicApi); // sync wrapper for STA

            if (apiResult.Success)
            {
                Logger.AddDebugLog("API check succeeded. Launching dashboard...");

                // Local HTML file path (change to your file)
                //                string dashboardUrl = "https://atomic-shield.com";
                string dashboardUrl = Path.Combine(Application.StartupPath, "ui.html");

                LoadEngine();

                Application.Run(new DashboardForm(_atomicApi.LoadClientUI("0000").Result));
            }
            else
            {
                Logger.AddDebugLog("API check failed: {0}", apiResult.Message);
                MessageBox.Show($"{apiResult.Title}\n\n{apiResult.Message}", "AtomicShield Agent",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private static void DeleteOldAgentIfExists(string cmdLine)
        {
            if (!cmdLine.Contains("--old")) return;

            int pos = cmdLine.IndexOf("--old", StringComparison.Ordinal);
            if (pos < 0) return;

            int endPos = cmdLine.IndexOf(' ', pos);
            string oldAgentPath = cmdLine.Substring(pos + 6, (endPos > 0 ? endPos : cmdLine.Length) - pos - 6);

            if (!string.IsNullOrWhiteSpace(oldAgentPath))
            {
                Logger.AddDebugLog("Deleting old agent at path: {0}", oldAgentPath);
                try
                {
                    File.Delete(oldAgentPath);
                }
                catch (Exception ex)
                {
                    Logger.AddDebugLog("Failed to delete old agent: {0}", ex.Message);
                }
            }
        }

        private static ApiCheckResult RunApiChecksSync(AtomicAPI atomicApi)
        {
            // Wrap async API check in synchronous STA-friendly call
            return Task.Run(async () => await RunApiChecksAsync(atomicApi))
                       .GetAwaiter()
                       .GetResult();
        }

        private static async Task<ApiCheckResult> RunApiChecksAsync(AtomicAPI atomicApi)
        {
            var result = new ApiCheckResult();

            try
            {
                result.Status = await atomicApi.GetStatusAsync();
                var root = result.Status.RootElement;

                if (root.TryGetProperty("alive", out System.Text.Json.JsonElement aliveProp) && !aliveProp.GetBoolean())
                {
                    result.Success = false;

                    if (root.TryGetProperty("title", out System.Text.Json.JsonElement titleProp))
                        result.Title = titleProp.GetString() ?? result.Title;

                    if (root.TryGetProperty("message", out System.Text.Json.JsonElement msgProp))
                        result.Message = msgProp.GetString() ?? result.Message;
                }
            }
            catch (Exception ex)
            {
                result.Success = false;
                result.Title = "API CHECK FAILED";
                result.Message = $"An error occurred during API checks: {ex.Message}";
                Logger.AddDebugLog("ApiChecks error: {0}", ex);
            }
            finally
            {
                result.Initialized = true;
            }

            return result;
        }
    }
}
