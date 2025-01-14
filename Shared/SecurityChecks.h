#pragma once
#include <Windows.h>

namespace SecurityChecks
{
    namespace AntiBreakpoint
    {
        static bool HasHardwareBreakpoint();
        static bool HasEntrypointBreakpoint();
        static bool HasMemoryBreakpoint();
    }

    namespace AntiDebug
    {
        static bool CheckCPUId();
    }

    namespace AntiDump
    {
        static bool InitializeAntiDump(HMODULE hModule);
        static bool IsDumpTriggered();

        /*static void ProtectSelfPE(HMODULE hModule);
        static void HideModuleLinks(HMODULE hModule);*/
    }
};
