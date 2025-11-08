using System;
using System.Diagnostics;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace AtomicAgent
{
    public class ServerEndPoint
    {
        public string Url { get; set; }
        public long LatencyMs { get; private set; } = -1;

        public ServerEndPoint(string url)
        {
            Url = url;
        }

        /// <summary>
        /// Measures latency to the server (TCP connect) with a timeout in milliseconds.
        /// </summary>
        public async Task MeasureLatencyAsync(int timeoutMs = 5000, int port = 443)
        {
            try
            {
                using (var client = new TcpClient())
                {
                    var sw = Stopwatch.StartNew();
                    var connectTask = client.ConnectAsync(Url, port);

                    if (await Task.WhenAny(connectTask, Task.Delay(timeoutMs)) == connectTask)
                    {
                        // Connected successfully
                        sw.Stop();
                        LatencyMs = sw.ElapsedMilliseconds;
                        Logger.AddDebugLog("Connection successful to {0} in {1} ms", Url, LatencyMs);
                    }
                    else
                    {
                        // Timeout
                        LatencyMs = -1;
                        Logger.AddDebugLog("Connection to {0} timed out after {1} ms", Url, timeoutMs);
                    }
                }
            }
            catch (Exception ex)
            {
                LatencyMs = -1;
                Logger.AddDebugLog("Connection failed to {0}: {1}", Url, ex.Message);
            }
        }
    }
}
