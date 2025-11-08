using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace AtomicAgent
{
    public class LatencyEvaluator
    {
        private readonly List<ServerEndPoint> _servers = new();

        private readonly AtomicEncoder _atomicCore;

        public LatencyEvaluator(AtomicEncoder atomicCore)
        {
            _atomicCore = atomicCore;
        }

        /// <summary>
        /// Adds a server to the evaluation list
        /// </summary>
        public void AddServer(string url)
        {
            _servers.Add(new ServerEndPoint(url));
        }

        /// <summary>
        /// Evaluates latency for all servers
        /// </summary>
        public async Task EvaluateAllAsync(int timeoutMs = 5000)
        {
            Logger.AddDebugLog("Evaluating server latencies ({0} endpoints)...", _servers.Count);

            foreach (var server in _servers)
            {
                await server.MeasureLatencyAsync(timeoutMs);
                Logger.AddDebugLog("Server: {0}, Latency: {1} ms", server.Url, server.LatencyMs);
            }
        }

        /// <summary>
        /// Returns the server with the lowest latency
        /// </summary>
        public ServerEndPoint GetBestServer()
        {
            return _servers.OrderBy(s => s.LatencyMs >= 0 ? s.LatencyMs : long.MaxValue).FirstOrDefault();
        }

        /// <summary>
        /// Caches the server endpoint in registry (encrypted & Base64)
        /// </summary>
        public void CacheEndPoint(string endpoint)
        {
            try
            {
                using var key = Registry.CurrentUser.CreateSubKey(@"Software\AtomicShield");
                if (key == null) return;

                string encrypted = Convert.ToBase64String(Encoding.UTF8.GetBytes(_atomicCore.Encrypt(endpoint)));
                key.SetValue("CachedEndPoint", encrypted, RegistryValueKind.String);
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Failed to cache endpoint: {0}", ex.Message);
            }
        }

        /// <summary>
        /// Retrieves the cached endpoint from registry
        /// </summary>
        public string GetCachedEndPoint()
        {
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(@"Software\AtomicShield");
                if (key == null) return string.Empty;

                string encrypted = key.GetValue("CachedEndPoint") as string;
                if (string.IsNullOrWhiteSpace(encrypted) || encrypted.Length < 5)
                    return string.Empty;

                return _atomicCore.Decrypt(Convert.FromBase64String(encrypted));

            }
            catch
            {
                return string.Empty;
            }
        }

        /// <summary>
        /// Sets up the best server endpoint
        /// </summary>
        public async Task SetupServerEndPointAsync(Action<string> endpointCallback, bool useCached = true)
        {
            string cached = GetCachedEndPoint();
            string bestEndpoint;

            if (!useCached || string.IsNullOrEmpty(cached))
            {
                // Add your servers here
                AddServer("31.97.180.157");
                AddServer("149.28.29.222");

                await EvaluateAllAsync();

                var bestServer = GetBestServer();
                if (bestServer == null)
                    throw new Exception("No available server found.");

                Logger.AddDebugLog("Nearest Server: {0}, Latency: {1} ms", bestServer.Url, bestServer.LatencyMs);

                CacheEndPoint(bestServer.Url);
                bestEndpoint = bestServer.Url;
            }
            else
            {
                Logger.AddDebugLog("Using cached endpoint: {0}", cached);
                bestEndpoint = cached;
            }

            endpointCallback?.Invoke("http://" + bestEndpoint);

#if ATOMIC_AGENT
            AtomicAPI.Instance.SetServerEndPoint(bestEndpoint);
#endif

#if ATOMIC_ENGINE
            Logger.AddDebugLog("Setting Server Endpoint: {0}", bestEndpoint);
            AtomicNetwork.Instance.SetServerEndPoint(bestEndpoint);
#endif
        }
    }
}
