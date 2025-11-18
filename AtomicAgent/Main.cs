using AtomicShield;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;
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
        public JsonDocument Status { get; set; }
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

                if (root.TryGetProperty("alive", out JsonElement aliveProp) && !aliveProp.GetBoolean())
                {
                    result.Success = false;

                    if (root.TryGetProperty("title", out JsonElement titleProp))
                        result.Title = titleProp.GetString() ?? result.Title;

                    if (root.TryGetProperty("message", out JsonElement msgProp))
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

    internal class DashboardForm : Form
    {
        private readonly WebView2 _webView;

        [DllImport("gdi32.dll", EntryPoint = "CreateRoundRectRgn")]
        private static extern IntPtr CreateRoundRectRgn(
            int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
            int nWidthEllipse, int nHeightEllipse);
        [DllImport("user32.dll")]
        public static extern bool ReleaseCapture();

        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, int Msg, int wParam, int lParam);

        private const int WM_NCLBUTTONDOWN = 0xA1;
        private const int HTCAPTION = 0x2;

        private void DashboardForm_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                ReleaseCapture();
                SendMessage(this.Handle, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
        }

        public DashboardForm(string uiBuffer)
        {

            var assembly = Assembly.GetExecutingAssembly();
            using (var stream = assembly.GetManifestResourceStream("AtomicAgent.favicon.ico")) // Namespace + filename
            {
                if (stream != null)
                {
                    this.Icon = new Icon(stream);
                }
            }
            Text = "AtomicShield Agent";
            Width = 662;
            Height = 500;

            StartPosition = FormStartPosition.CenterScreen;
            this.Region = Region.FromHrgn(CreateRoundRectRgn(0, 0, Width, Height, 20, 20));
            BackColor = Color.FromArgb(12, 12, 12);

            FormBorderStyle = FormBorderStyle.None;
            this.Resize += DashboardForm_Resize;

            Name = "AtomicAgent";
            

            _webView = new WebView2
            {
                Dock = DockStyle.Fill,
                BackColor = Color.FromArgb(12, 12, 12) // SAME as form

            };
            Controls.Add(_webView);

            Load += (s, e) => InitializeWebView(uiBuffer);
        }
        private void DashboardForm_Resize(object? sender, EventArgs e)
        {
            if (Width > 0 && Height > 0)
            {
                var hRgn = CreateRoundRectRgn(0, 0, Width, Height, 20, 20);
                this.Region = Region.FromHrgn(hRgn);
            }
        }
        private async void InitializeWebView(string uiBuffer)
        {
            try
            {
                await _webView.EnsureCoreWebView2Async(null);

                _webView.CoreWebView2.Settings.AreDevToolsEnabled = false;
                _webView.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
                _webView.CoreWebView2.Settings.IsZoomControlEnabled = false;
                _webView.CoreWebView2.Settings.IsStatusBarEnabled = false;
                _webView.CoreWebView2.WebMessageReceived += CoreWebView2_WebMessageReceived;
           //     _webView.DefaultBackgroundColor = System.Drawing.Color.Transparent;


                _webView.CoreWebView2.NavigateToString(uiBuffer);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to initialize WebView2: {ex.Message}", "Error",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void CoreWebView2_WebMessageReceived(object sender, CoreWebView2WebMessageReceivedEventArgs e)
        {
            string json = e.WebMessageAsJson;
            if (string.IsNullOrWhiteSpace(json)) return;

            try
            {
                using var doc = JsonDocument.Parse(json);
                if (!doc.RootElement.TryGetProperty("action", out var actionEl)) return;
                string action = actionEl.GetString();

                this.Invoke((Action)(() =>
                {
                    switch (action)
                    {
                        case "minimize":
                            this.WindowState = FormWindowState.Minimized;
                            break;

                        case "close":
                            this.Close();
                            break;

                        case "enableShield":
                            SendCommandToPage(new { cmd = "showToast", msg = "Shield enabled by host" });
                            break;
                        case "drag":
                            ReleaseCapture();
                            SendMessage(this.Handle, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                            break;

                        default:
                            break;
                    }
                }));
            }
            catch (Exception ex)
            {
                Debug.WriteLine("WebMessage parse error: " + ex);
            }
        }
        private void SendCommandToPage(object payload)
        {
            try
            {
                string json = JsonSerializer.Serialize(payload);
                _webView.CoreWebView2.PostWebMessageAsJson(json);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("SendCommandToPage error: " + ex);
            }
        }

        private async Task CallJsEnableShield()
        {
            await _webView.CoreWebView2.ExecuteScriptAsync("enableShield();");
        }
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            _webView.Dispose();
            base.OnFormClosing(e);
        }
    }
}
