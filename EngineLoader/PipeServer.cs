using System;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Threading.Tasks;
using Shared.Utils;
using EngineLoader.Loader;

namespace EngineLoader
{
    public class PipeServer
    {
        public static async Task Launch()
        {
            Logger.AddDebugLog("Launching pipe server...");

            while (true)
            {
                // Create a new pipe for the next client BEFORE handling the current client
                var server = new NamedPipeServerStream(
                    "AtomicPipe",
                    PipeDirection.InOut,
                    NamedPipeServerStream.MaxAllowedServerInstances,
                    PipeTransmissionMode.Byte,
                    PipeOptions.Asynchronous
                );

                // Accept connection asynchronously
                await server.WaitForConnectionAsync();

                // Immediately spawn task to handle this client
                _ = HandleClientAsync(server);
            }
        }

        private static async Task HandleClientAsync(NamedPipeServerStream server)
        {
            try
            {
                Logger.AddDebugLog("Client connected");

                using (server)
                using (var reader = new StreamReader(server, Encoding.UTF8))
                using (var writer = new StreamWriter(server, Encoding.UTF8) { AutoFlush = true })
                {
                    while (server.IsConnected)
                    {
                        string message = await reader.ReadLineAsync();
                        if (message == null)
                        {
                            Logger.AddDebugLog("Client disconnected");
                            break;
                        }

                        Logger.AddDebugLog("Received: " + message);

                        if (message.StartsWith("load_code:"))
                        {
                            string codeToLoad = message.Substring("load_code:".Length);

                            try
                            {
                                var codeBuffer = Convert.FromBase64String(codeToLoad);

                                using var mapper = new InProcessManualMapper();
                                var basePtr = mapper.Map(codeBuffer, true);

                                if (basePtr == IntPtr.Zero)
                                {
                                    Logger.AddDebugLog("Failed to map the provided code.");
                                    await writer.WriteLineAsync("load_failure:");
                                }
                                else
                                {
                                    Logger.AddDebugLog("Dynamically loaded code from pipe.");
                                    await writer.WriteLineAsync("load_success:");
                                }
                            }
                            catch (Exception ex)
                            {
                                Logger.AddDebugLog("Failed to load code from pipe: " + ex.Message);
                                await writer.WriteLineAsync("load_failure:" + ex.Message);
                            }
                        }
                    }
                }
            }
            catch (IOException ex)
            {
                Logger.AddDebugLog("Pipe client handler exception: " + ex.Message);
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Unexpected exception: " + ex.Message);
            }
        }
    }
}
