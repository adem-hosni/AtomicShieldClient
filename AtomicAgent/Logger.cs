using System;
using System.IO;
using System.Text;
using System.Threading;

namespace AtomicAgent
{
    public static class Logger
    {
        private static readonly object _lock = new object();
        private static readonly string LogFolder;
        private static readonly string LogFile;

        static Logger()
        {
            string appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            LogFolder = Path.Combine(appData, "AtomicShield");
            Directory.CreateDirectory(LogFolder);
            LogFile = Path.Combine(LogFolder, "AtomicAgent.log");
        }

        /// <summary>
        /// Writes a formatted debug log with timestamp
        /// </summary>
        public static void AddDebugLog(string message, params object[] args)
        {
            try
            {
                string formattedMessage = args.Length > 0 ? string.Format(message, args) : message;
                string logEntry = $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff}] {formattedMessage}";

                lock (_lock)
                {
                    File.AppendAllText(LogFile, logEntry + Environment.NewLine, Encoding.UTF8);
                }

#if DEBUG
                Console.WriteLine(logEntry); // Optional: output to console in debug builds
#endif
            }
            catch
            {
                // Swallow exceptions to avoid crashing the agent
            }
        }

        /// <summary>
        /// Clears the log file
        /// </summary>
        public static void ClearLog()
        {
            try
            {
                lock (_lock)
                {
                    if (File.Exists(LogFile))
                        File.WriteAllText(LogFile, string.Empty, Encoding.UTF8);
                }
            }
            catch
            {
                // Swallow exceptions
            }
        }
    }
}
