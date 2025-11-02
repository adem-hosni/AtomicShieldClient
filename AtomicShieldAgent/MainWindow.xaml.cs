using AtomicAgent;
using Microsoft.Web.WebView2.Core;
using System;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;

namespace AtomicShieldAgent
{
    internal class ApiCheckResult
    {
        public bool Success { get; set; } = true;
        public bool Initialized { get; set; } = false;
        public string Title { get; set; } = "Loading Content Manifest...";
        public string Message { get; set; } = "The agent is loading. This won't take long.";
        public JsonDocument Status { get; set; }
    }

    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            var atomicApi = new AtomicAPI(new AtomicEncoder());

            ApiCheckResult apiResult = RunApiChecksSync(atomicApi);

            if (apiResult.Success)
            {
                Logger.AddDebugLog("API check succeeded. Launching dashboard...");
                string dashboardUrl = "https://atomic-shield.com";

                _ = InitializeWebViewAndNavigateAsync(dashboardUrl);
            }
            else
            {
                Logger.AddDebugLog("API check failed: {0}", apiResult.Message);
                MessageBox.Show($"{apiResult.Title}\n\n{apiResult.Message}", "AtomicShield Agent",
                    MessageBoxButton.OK, MessageBoxImage.Error);

                Close();
            }

            try
            {
                DeleteOldAgentIfExists(Environment.CommandLine);
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("DeleteOldAgentIfExists failed: {0}", ex.Message);
            }
        }

        private static ApiCheckResult RunApiChecksSync(AtomicAPI atomicApi)
        {
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

        private async Task InitializeWebViewAndNavigateAsync(string url)
        {
            try
            {
                if (WebView == null)
                {
                    Logger.AddDebugLog("WebView control not found in XAML.");
                    MessageBox.Show("WebView control not found. Make sure MainWindow.xaml contains a WebView2 named 'WebView'.",
                        "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    Close();
                    return;
                }

                await WebView.EnsureCoreWebView2Async();

#if DEBUG
                // helpful during development
                WebView.CoreWebView2.OpenDevToolsWindow();
#endif
                if (Uri.TryCreate(url, UriKind.Absolute, out var parsed))
                {
                    if (parsed.Scheme == "file")
                        WebView.CoreWebView2.Navigate(parsed.AbsoluteUri);
                    else
                        WebView.CoreWebView2.Navigate(url);
                }
                else
                {
                    var localPath = Path.Combine(AppContext.BaseDirectory, url);
                    if (File.Exists(localPath))
                    {
                        var fileUri = new Uri(localPath);
                        WebView.CoreWebView2.Navigate(fileUri.AbsoluteUri);
                    }
                    else
                    {
                        WebView.CoreWebView2.Navigate(url);
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Failed to initialize WebView2: {0}", ex.Message);
                MessageBox.Show($"Failed to initialize WebView2: {ex.Message}", "Error",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                Close();
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            try
            {
                if (WebView?.CoreWebView2 != null)
                {
                    WebView.CoreWebView2.WebMessageReceived -= CoreWebView2_WebMessageReceived;
                }
                WebView?.Dispose();
            }
            catch { /* ignore */ }

            base.OnClosed(e);
        }

        private void CoreWebView2_WebMessageReceived(object sender, CoreWebView2WebMessageReceivedEventArgs e)
        {
            try
            {
                var msg = e.TryGetWebMessageAsString();
                if (string.IsNullOrEmpty(msg)) return;

                switch (msg.ToLowerInvariant())
                {
                    case "start":
                        Logger.AddDebugLog("Start command received from web UI");
                        WebView.CoreWebView2.PostWebMessageAsString("Agent started");
                        break;
                    case "stop":
                        Logger.AddDebugLog("Stop command received from web UI");
                        WebView.CoreWebView2.PostWebMessageAsString("Agent stopped");
                        break;
                    default:
                        WebView.CoreWebView2.PostWebMessageAsString("Unknown command: " + msg);
                        break;
                }
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Error in WebMessageReceived: {0}", ex.Message);
            }
        }
    }
}
