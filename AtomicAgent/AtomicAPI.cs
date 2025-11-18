using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

public class UserData
{
    public float Progress { get; set; }
}

namespace AtomicAgent
{
    public class AtomicAPI
    {
        private const string API_BASE_URL = "http://31.97.180.157:8002"; // Replace with your actual URL
        private string serverEndPoint;
        private AtomicEncoder AtomicEncoder; // Reference to your CAtomicEncoder equivalent
        private HttpClient httpClient;

        public AtomicAPI(AtomicEncoder core)
        {
            serverEndPoint = API_BASE_URL;
            AtomicEncoder = core;
            httpClient = new HttpClient();
            httpClient.Timeout = TimeSpan.FromSeconds(30);
        }

        public async Task<JsonDocument> GetStatusAsync()
        {
            string url = serverEndPoint + "/anticheat/status/agent";

            try
            {
                var buffer = await PostRequestAsync(url, null);
                if (string.IsNullOrEmpty(buffer))
                {
                    var errorJson = new
                    {
                        alive = false,
                        title = "Connection Error",
                        message = "Failed to connect to AtomicShield Server"
                    };
                    string serialized = JsonSerializer.Serialize(errorJson);
                    return JsonDocument.Parse(serialized);
                }
                return JsonDocument.Parse(buffer);
            }
            catch
            {
                var errorJson = new
                {
                    alive = false,
                    title = "Connection Error",
                    message = "Failed to connect to AtomicShield Server"
                };
                string serialized = JsonSerializer.Serialize(errorJson);
                return JsonDocument.Parse(serialized);
            }
        }

        public async Task<bool> IsAlreadyConnectedAsync()
        {
            string url = serverEndPoint + "/anticheat/status/isconnected";
            var buffer = await PostRequestAsync(url, null);
            if (string.IsNullOrEmpty(buffer)) return false;

            var json = JsonDocument.Parse(buffer);
            return json.RootElement.GetProperty("success").GetBoolean();
        }

        public async Task<bool> IsValidVersionAsync(string version)
        {
            string url = serverEndPoint + "/anticheat/status/version";
            var requestBody = new { version };
            var buffer = await PostRequestAsync(url, requestBody);
            if (string.IsNullOrEmpty(buffer)) return false;

            var json = JsonDocument.Parse(buffer);
            return json.RootElement.GetProperty("success").GetBoolean();
        }

        public async Task<string> DownloadEngineAsync(UserData userData = null)
        {
            string url = serverEndPoint + "/resources/scan/fivem";
            return await PostRequestAsync(url, null, userData);
        }

        public async Task DownloadLatestAgentAsync()
        {
            string url = serverEndPoint + "/resources/latest-agent";
            await PostRequestAsync(url, null, null, encryptRequest: false, decryptResponse: false);
        }

        public async Task<string> LoadClientUI(string code)
        {
            string url = serverEndPoint + "/resources/client-ui";
            return await PostRequestAsync(url, new { code = code }, null, encryptRequest: false, decryptResponse: false);
        }

        public async Task<string> DownloadClientLoader()
        {
            string url = serverEndPoint + "/resources/client-loader";
            return await PostRequestAsync(url, null, null, encryptRequest: false, decryptResponse: true);
        }

        private async Task<string> PostRequestAsync(string url, object data, UserData userData = null, bool encryptRequest = true, bool decryptResponse = true)
        {
            string requestBody = data != null ? JsonSerializer.Serialize(data) : "";
            if (encryptRequest && !string.IsNullOrEmpty(requestBody))
            {
                if (requestBody.Length >= 16)
                    requestBody = Convert.ToBase64String(Encoding.UTF8.GetBytes(AtomicEncoder.Encrypt(requestBody)));
                else
                    requestBody = Convert.ToBase64String(Encoding.UTF8.GetBytes(requestBody));
            }

            using (var content = new StringContent(requestBody, Encoding.UTF8, "application/json"))
            {
                HttpResponseMessage response;
                try
                {
                    response = await httpClient.PostAsync(url, content);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Request failed: {ex.Message}");
                    return "";
                }

                string responseString = await response.Content.ReadAsStringAsync();

                if (decryptResponse && !string.IsNullOrEmpty(responseString))
                {
                    string decrypted = AtomicEncoder.Decrypt(Convert.FromBase64String(responseString));
                    return decrypted;
                }

                return responseString;
            }
        }
    }
}