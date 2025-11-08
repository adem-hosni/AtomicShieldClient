using System;
using System.Diagnostics;
using Microsoft.Win32.TaskScheduler;

namespace AtomicAgent
{
    public static class StartupManager
    {
        private const string TaskName = "AtomicAgentStartup";

        /// <summary>
        /// Adds the current app to Windows Task Scheduler to start at login
        /// </summary>
        public static bool AddAppToTaskScheduler()
        {
            try
            {
                using TaskService ts = new TaskService();
                TaskDefinition td = ts.NewTask();
                td.RegistrationInfo.Description = "Start AtomicAgent on Windows startup";

                // Trigger: logon
                td.Triggers.Add(new LogonTrigger());

                // Action: start this executable
                td.Actions.Add(new ExecAction(Process.GetCurrentProcess().MainModule.FileName));

                ts.RootFolder.RegisterTaskDefinition(TaskName, td);
                Logger.AddDebugLog("Successfully added app to Task Scheduler.");
                return true;
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Failed to add app to Task Scheduler: {0}", ex.Message);
                return false;
            }
        }

        /// <summary>
        /// Removes the app from Task Scheduler
        /// </summary>
        public static bool RemoveAppFromTaskScheduler()
        {
            try
            {
                using TaskService ts = new TaskService();
                ts.RootFolder.DeleteTask(TaskName, false);
                Logger.AddDebugLog("Successfully removed app from Task Scheduler.");
                return true;
            }
            catch (Exception ex)
            {
                Logger.AddDebugLog("Failed to remove app from Task Scheduler: {0}", ex.Message);
                return false;
            }
        }

        /// <summary>
        /// Checks if the app is already in Task Scheduler
        /// </summary>
        public static bool IsAppInTaskScheduler()
        {
            try
            {
                using TaskService ts = new TaskService();
                var task = ts.GetTask(TaskName);
                return task != null;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Returns the current process executable name
        /// </summary>
        public static string GetCurrentProcessName()
        {
            try
            {
                return Process.GetCurrentProcess().ProcessName;
            }
            catch
            {
                return string.Empty;
            }
        }

        /// <summary>
        /// Convenience method to execute startup logic
        /// </summary>
        public static void StartupFunction()
        {
            if (!IsAppInTaskScheduler())
            {
                AddAppToTaskScheduler();
            }
        }
    }
}
