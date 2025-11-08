using System;
using System.Runtime.InteropServices;

namespace AtomicAgent.ManualMapper
{
    internal class ManualMapper
    {
        public static int ManualMapModule(IntPtr hProcess, byte[] moduleBuffer)
        {
            if (hProcess == IntPtr.Zero)
            {
                Logger.AddDebugLog("ManualMapModule: invalid process handle (IntPtr.Zero).");
                return -1;
            }

            if (moduleBuffer == null || moduleBuffer.Length == 0)
            {
                Logger.AddDebugLog("ManualMapModule: moduleBuffer is null or empty.");
                return -2;
            }

            IntPtr pTargetBase = WinAPI.Memory.VirtualAllocEx(
                hProcess,
                IntPtr.Zero,
                (uint)moduleBuffer.Length,
                WinAPI.Memory.AllocationType.Commit | WinAPI.Memory.AllocationType.Reserve,
                WinAPI.Memory.MemoryProtection.ReadWrite);

            if (pTargetBase == IntPtr.Zero)
            {
                int err = WinAPI.Memory.GetLastError();
                Logger.AddDebugLog("ManualMapModule: VirtualAllocEx failed. Error: {0}", err);
                return -3;
            }

            WinAPI.Memory.VirtualProtectEx(hProcess, pTargetBase, (uint)moduleBuffer.Length, WinAPI.Memory.MemoryProtection.ExecuteReadWrite, out _);

            bool written = WinAPI.Memory.WriteProcessMemory(
                hProcess,
                pTargetBase,
                moduleBuffer,
                (uint)moduleBuffer.Length,
                out IntPtr bytesWritten);

            if (!written || bytesWritten == IntPtr.Zero)
            {
                int err = WinAPI.Memory.GetLastError();
                Logger.AddDebugLog("ManualMapModule: WriteProcessMemory failed. Error: {0}", err);
                WinAPI.Memory.VirtualFreeEx(hProcess, pTargetBase, UIntPtr.Zero, WinAPI.Memory.FreeType.Release);
                return -4;
            }

            bool protectOk = WinAPI.Memory.VirtualProtectEx(
                hProcess,
                pTargetBase,
                (uint)moduleBuffer.Length,
                WinAPI.Memory.MemoryProtection.ExecuteRead,
                out _);

            if (!protectOk)
            {
                int err = WinAPI.Memory.GetLastError();
                Logger.AddDebugLog("ManualMapModule: VirtualProtectEx failed. Error: {0}", err);
            }

            Logger.AddDebugLog("ManualMapModule: module mapped at {0} (size {1} bytes).", pTargetBase, moduleBuffer.Length);
            return 0;
        }
    }
}
