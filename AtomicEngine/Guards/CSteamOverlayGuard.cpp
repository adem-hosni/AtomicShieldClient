#include "StdInc.h"
#include "CSteamOverlayGuard.h"
#include "KernelCalls.hpp"

CSteamOverlayGuard::CSteamOverlayGuard()
{
}

CSteamOverlayGuard::~CSteamOverlayGuard()
{
}

void CSteamOverlayGuard::DoPulse()
{
    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL || g_pAtomicAntiCheat->GetProcessHandle() == INVALID_HANDLE_VALUE ||
               g_pAtomicAntiCheat->GetProcessHandle() == NULL)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        MODULEENTRY32 ModuleEntry = Utils::GetModuleEntry("GameOverlayRenderer64.dll", g_pAtomicAntiCheat->GetProcessID());
        DWORD64       dwSteamModuleBase = (DWORD64)ModuleEntry.modBaseAddr;

        if (!dwSteamModuleBase || ModuleEntry.hModule == NULL)
        {
            SharedUtil::AddDebugLog("GameOverlayRenderer64.dll not found in process memory");
            continue;
        }
        SharedUtil::AddDebugLog("GameOverlayRenderer64.dll Found at 0x%p", dwSteamModuleBase);

        LPVOID lpModuleBuffer = NULL;

        MODULEINFO ModuleInfo;
        if (!GetModuleInformation(g_pAtomicAntiCheat->GetProcessHandle(), ModuleEntry.hModule, &ModuleInfo, sizeof(MODULEINFO)))
        {
            SharedUtil::AddDebugLog("Failed to get module information for GameOverlayRenderer64.dll");
            continue;
        }

        DWORD64  dwAllocationSize = ModuleInfo.SizeOfImage;
        NTSTATUS status = SysNtAllocateVirtualMemory(GetCurrentProcess(), &lpModuleBuffer, 0, &dwAllocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!NT_SUCCESS(status) || lpModuleBuffer == NULL)
        {
            SharedUtil::AddDebugLog("Failed to allocate memory for GameOverlayRenderer64.dll");
            continue;
        }

        size_t bytesRead = NULL;
        status = SysNtReadVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), (LPVOID)dwSteamModuleBase, lpModuleBuffer, ModuleInfo.SizeOfImage, &bytesRead);
        if (!NT_SUCCESS(status) || bytesRead == 0)
        {
            SharedUtil::AddDebugLog("Failed to read GameOverlayRenderer64.dll memory (Error 0x%x, status: 0x%016llX)", status, status);
            SysNtFreeVirtualMemory(GetCurrentProcess(), &lpModuleBuffer, &dwAllocationSize, MEM_RELEASE);
            continue;
        }

        FILE* hFile = fopen("memdump.dll", "wb");
        if (hFile)
        {
            fwrite(lpModuleBuffer, 1, ModuleInfo.SizeOfImage, hFile);
            fclose(hFile);
        }
        else
        {
            SharedUtil::AddDebugLog("Failed to open memdump.dll for writing");
        }

        SharedUtil::AddDebugLog("Read %zu bytes from GameOverlayRenderer64.dll at 0x%p", bytesRead, lpModuleBuffer);

        DWORD64 dwPresentDXGILoc = (DWORD64)MemoryScanner::FindPattern((uint8_t*)lpModuleBuffer, bytesRead,
                                                                       "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 20 41 8B E8");
        SharedUtil::AddDebugLog("Scanner PresentDXGI: 0x%p - Hooked: %d", dwPresentDXGILoc,
                                Utils::IsFunctionHooked((const char*)lpModuleBuffer, bytesRead, dwPresentDXGILoc));

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}