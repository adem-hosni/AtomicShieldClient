#include "StdInc.h"
#include "KernelCalls.hpp"
#include "Structs.h"

CManualMappingGuard::CManualMappingGuard()
{
}

CManualMappingGuard::~CManualMappingGuard()
{
}

void CManualMappingGuard::Initialize()
{
}

bool CManualMappingGuard::IsModuleLoaded(DWORD64 dwModuleBase)
{
    struct PROCESS_BASIC_INFORMATION
    {
        PVOID     Reserved1;
        PPEB      PebBaseAddress;
        PVOID     Reserved2[2];
        ULONG_PTR UniqueProcessId;
        PVOID     Reserved3;
    };

    PROCESS_BASIC_INFORMATION pbi = {};
    ULONG                     retLen = 0;
    HANDLE                    hProcess = g_pAtomicAntiCheat->GetProcessHandle();

    if (!NT_SUCCESS(SysNtQueryInformationProcess(hProcess, KernelCalls_ProcessBasicInformation, &pbi, sizeof(pbi), &retLen)) || !pbi.PebBaseAddress)
    {
        MANUALMAP_LOG("Failed to query process information or PEB is null.");
        return false;
    }

    // Read remote PEB
    Sys_PEB peb = {};
    SIZE_T  bytesRead = 0;
    if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &bytesRead)) || bytesRead == 0)
    {
        MANUALMAP_LOG("Failed to read PEB at address %p, error: %d", pbi.PebBaseAddress, GetLastError());
        return false;
    }

    if (!peb.Ldr)
    {
        MANUALMAP_LOG("PEB.Ldr is null.");
        return false;
    }

    // Remote address of InMemoryOrderModuleList
    LIST_ENTRY* pListHeadRemote = (LIST_ENTRY*)((BYTE*)peb.Ldr + offsetof(Sys_PEB_LDR_DATA, InMemoryOrderModuleList));

    LIST_ENTRY currentEntry = {};
    if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, pListHeadRemote, &currentEntry, sizeof(currentEntry), &bytesRead)) || bytesRead == 0)
    {
        MANUALMAP_LOG("Failed to read InMemoryOrderModuleList at address %p, error: %d", pListHeadRemote, GetLastError());
        return false;
    }

    LIST_ENTRY* pFlink = currentEntry.Flink;

    while (pFlink && pFlink != pListHeadRemote)
    {
        auto pLdrEntryRemote = (Sys_LDR_DATA_TABLE_ENTRY*)((BYTE*)pFlink - offsetof(Sys_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        Sys_LDR_DATA_TABLE_ENTRY ldrEntry = {};
        if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, pLdrEntryRemote, &ldrEntry, sizeof(ldrEntry), &bytesRead)) || bytesRead == 0)
        {
            MANUALMAP_LOG("Failed to read LDR_DATA_TABLE_ENTRY at address %p, error: %d", pLdrEntryRemote, GetLastError());
            break;
        }

        if ((DWORD64)ldrEntry.DllBase == dwModuleBase)
        {
            return true;
        }

        pFlink = ldrEntry.InMemoryOrderLinks.Flink;
    }

    return false;
}

DWORD64 CManualMappingGuard::GetPEHeaderSize(DWORD64 dwBaseAddress)
{
    IMAGE_DOS_HEADER dosHeader = {};
    SIZE_T           bytesRead = 0;
    HANDLE           hProcess = g_pAtomicAntiCheat->GetProcessHandle();

    if (SysNtReadVirtualMemory(hProcess, (PVOID)dwBaseAddress, &dosHeader, sizeof(dosHeader), &bytesRead) != 0 || bytesRead != sizeof(dosHeader))
        return 0;

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    IMAGE_NT_HEADERS ntHeader = {};
    DWORD64          ntHeaderAddr = dwBaseAddress + dosHeader.e_lfanew;

    if (SysNtReadVirtualMemory(hProcess, (PVOID)ntHeaderAddr, &ntHeader, sizeof(ntHeader), &bytesRead) != 0 || bytesRead != sizeof(ntHeader))
        return 0;

    if (ntHeader.Signature != IMAGE_NT_SIGNATURE)
        return 0;

    if (!ntHeader.FileHeader.SizeOfOptionalHeader)
        return 0;

    return ntHeader.FileHeader.SizeOfOptionalHeader;
}

void CManualMappingGuard::DoPulse()
{
    MANUALMAP_LOG(__FUNCTION__ " Called");

    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    NTSTATUS                 status;
    MEMORY_BASIC_INFORMATION MemoryRegion{};
    SYSTEM_INFO              sysInfo;
    GetSystemInfo(&sysInfo);

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (!g_pAtomicAntiCheat->IsValidProcessHandle())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        for (LPVOID lpCurrentAddress = sysInfo.lpMinimumApplicationAddress; lpCurrentAddress < sysInfo.lpMaximumApplicationAddress;
             lpCurrentAddress = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize)
        {
            size_t returnLength = 0;
            if (!NT_SUCCESS(status = SysNtQueryVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), lpCurrentAddress, MemoryBasicInformation, &MemoryRegion,
                                                             sizeof(MemoryRegion), &returnLength)))
            {
                MANUALMAP_LOG("Failed to query memory region at address 0x%llx, error: %d", reinterpret_cast<DWORD64>(lpCurrentAddress), status);
                continue;
            }

            if (!MemoryRegion.AllocationBase || lpCurrentAddress != MemoryRegion.AllocationBase)
                continue;

            if (!(MemoryRegion.State & MEM_COMMIT) || MemoryRegion.State & MEM_RELEASE)
                continue;

            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
                continue;

            if (MemoryRegion.Type != MEM_PRIVATE)
                continue;

            const DWORD64 dwPEHeaderSize = GetPEHeaderSize(reinterpret_cast<DWORD64>(lpCurrentAddress));

            size_t               bytesRead = NULL;
            std::vector<uint8_t> buffer(dwPEHeaderSize);
            status = SysNtReadVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), lpCurrentAddress, buffer.data(), dwPEHeaderSize, &bytesRead);
            if (!NT_SUCCESS(status))
            {
                MANUALMAP_LOG("Failed to read memory at address 0x%llx, error: 0x%llx", reinterpret_cast<DWORD64>(lpCurrentAddress), status);
                continue;
            }

            char szFileName[MAX_PATH] = {0};
            GetMappedFileNameA(g_pAtomicAntiCheat->GetProcessHandle(), lpCurrentAddress, szFileName, sizeof(szFileName));

            auto wStrFileName = std::wstring(szFileName, szFileName + strlen(szFileName));

            bool                 bHasLoaded = IsModuleLoaded(reinterpret_cast<DWORD64>(lpCurrentAddress));
            std::vector<uint8_t> nullBuffer(dwPEHeaderSize, 0x00);

            bool bHasErasedHeader = !memcmp(buffer.data(), nullBuffer.data(), dwPEHeaderSize);

            if ((!bHasLoaded) || bHasErasedHeader)
            {
                MANUALMAP_LOG(
                    "A MMM Detected at 0x%llx with size %zu and memory type 0x%llx, base address: 0x%llX file "
                    "%s, has loaded: %d, has erased header: %d",
                    reinterpret_cast<DWORD64>(lpCurrentAddress), MemoryRegion.RegionSize, MemoryRegion.Type, reinterpret_cast<DWORD64>(MemoryRegion.BaseAddress), szFileName,
                    bHasLoaded, bHasErasedHeader);
            }
        }

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        MANUALMAP_LOG("Manual Mapping Guard Pulse completed in %.3f seconds", fElapsedTime);

        std::this_thread::sleep_for(std::chrono::seconds(4));
    }
    _endthreadex(0);
}