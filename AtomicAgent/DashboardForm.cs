using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AtomicAgent
{
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
            using (var stream = assembly.GetManifestResourceStream("AtomicAgent.favicon.ico"))
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
                BackColor = Color.FromArgb(12, 12, 12)
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