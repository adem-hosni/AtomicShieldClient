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

    if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, (PVOID)dwBaseAddress, &dosHeader, sizeof(dosHeader), &bytesRead)) || bytesRead == 0)
        return 0;

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    IMAGE_NT_HEADERS ntHeader = {};
    DWORD64          ntHeaderAddr = dwBaseAddress + dosHeader.e_lfanew;

    if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, (PVOID)ntHeaderAddr, &ntHeader, sizeof(ntHeader), &bytesRead)) || bytesRead != sizeof(ntHeader))
        return 0;

    if (ntHeader.Signature != IMAGE_NT_SIGNATURE)
        return 0;

    if (!ntHeader.FileHeader.SizeOfOptionalHeader)
        return 0;

    return ntHeader.FileHeader.SizeOfOptionalHeader;
}

bool CManualMappingGuard::GetCodeSectionAddress(DWORD64 dwModuleBase, DWORD64& sectionStart, DWORD64& sectionSize, NTSTATUS& status)
{
    MEMORY_BASIC_INFORMATION mbi = {0};
    SIZE_T                   len = 0;
    PVOID                    currentAddr = (PVOID)dwModuleBase;
    HANDLE                   hProcess = GetCurrentProcess();            // Or whatever process handle you're using

    while (true)
    {
        status = SysNtQueryVirtualMemory(hProcess, currentAddr, MemoryBasicInformation, &mbi, sizeof(mbi), &len);

        if (!NT_SUCCESS(status))
            break;

        // Exit if we've moved outside our module's memory
        if ((DWORD64)mbi.AllocationBase != dwModuleBase)
            break;

        // Check for committed, MEM_IMAGE, and executable protection
        if ((mbi.State == MEM_COMMIT) && (mbi.Type == MEM_IMAGE) &&
            (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        {
            sectionStart = (DWORD64)mbi.BaseAddress;
            sectionSize = (DWORD64)mbi.RegionSize;
            return true;
        }

        // Move to next memory region
        currentAddr = (PBYTE)mbi.BaseAddress + mbi.RegionSize;
    }

    return false;
}

bool CManualMappingGuard::ScanRegionForIATThunk(BYTE* pBuffer, size_t regionSize, DWORD64 dwBaseAddress)
{
    SIZE_T  complete_sequence = 0;
    DWORD64 foundIAT = 0;

    for (SIZE_T z = 0; z < regionSize; ++z)
    {
        for (SIZE_T x = 0; x < (8 * 6); x += 0x6)
        {
            SIZE_T offset = z + x;

            if ((offset + 1) < regionSize)
            {
                if (pBuffer[offset] == 0xFF && pBuffer[offset + 1] == 0x25)
                {
                    foundIAT = dwBaseAddress + offset;
                    complete_sequence++;
                }
                else
                {
                    complete_sequence = 0;
                }
            }
            else
            {
                complete_sequence = 0;
            }

            if (complete_sequence >= 8)
            {
                return true;
            }
        }
    }

    return false;
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

    auto MemoryMap = Utils::BuildModuledMemoryMap(g_pAtomicAntiCheat->GetProcessHandle());
    if (MemoryMap.size() == 0)
    {
        MANUALMAP_LOG("Failed to build module map! error: 0x%llx", GetLastError());
    }

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

            /*if (MemoryRegion.Type != MEM_PRIVATE)
                continue;*/

            if (MemoryRegion.RegionSize < 1024 * 1024)
                continue;

            if (MemoryRegion.State != MEM_COMMIT)
                continue;

            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
                continue;

            if (!(MemoryRegion.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
                continue;

            // Region is executable?
            if (!(MemoryRegion.Protect & PAGE_EXECUTE_READ || MemoryRegion.Protect & PAGE_EXECUTE_READWRITE))
                continue;

            const DWORD64 dwPEHeaderSize = 0x1000;            // GetPEHeaderSize(reinterpret_cast<DWORD64>(lpCurrentAddress));
            /*if (dwPEHeaderSize == NULL)
            {
                MANUALMAP_LOG("PE Header size is 0, looking for another region...");
                continue;
            }*/

            size_t               bytesRead = NULL;
            std::vector<uint8_t> buffer(dwPEHeaderSize);
            status = SysNtReadVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), lpCurrentAddress, buffer.data(), dwPEHeaderSize, &bytesRead);
            if (!NT_SUCCESS(status) || bytesRead == NULL)
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

            if ((!bHasLoaded || bHasErasedHeader) && Utils::IsAddressInModuledRange(reinterpret_cast<DWORD64>(lpCurrentAddress), MemoryMap))
            {
                MANUALMAP_LOG("Suspicious module found at 0x%llx | Erased Header: %d, Loaded: %d", lpCurrentAddress, bHasErasedHeader, bHasLoaded);

                DWORD64 dwTextSectionStart = NULL;
                DWORD64 dwTextSectionSize = NULL;

                /*if (GetCodeSectionAddress(reinterpret_cast<DWORD64>(lpCurrentAddress), dwTextSectionStart, dwTextSectionSize, status))
                {
                    std::vector<BYTE> TextSectionBuffer(dwTextSectionSize);
                    if (!NT_SUCCESS(status = SysNtReadVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), lpCurrentAddress, TextSectionBuffer.data(),
                                                                    dwTextSectionSize, &bytesRead)) ||
                        bytesRead == NULL)
                    {
                        MANUALMAP_LOG("Failed to read text section vmemory, error: 0x%llx", status);
                        continue;
                    }

                    const char pattern[] =
                        "\x48\x8B\xC4\x48\x89\x58\x20\x4C\x89\x40\x18\x89\x50\x10\x48\x89\x48\x08\x56\x57\x41\x56\x48\x83\xEC\x40\x49\x8B\xF0\x8B\xFA\x4C\x8B"
                        "\xF1\x85\xD2\x75\x0F\x39\x15\x00\x00\x00\x00\x7F\x07\x33\xC0\xE9\x00\x00\x00\x00";
                    const char wildcard[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxx????";

                    size_t patternLength = strlen(wildcard);

                    for (size_t z = 0; z < dwTextSectionSize - patternLength; z++)
                    {
                        bool bFound = true;
                        for (size_t j = 0; j < patternLength; j++)
                        {
                            if (wildcard[j] != 'j' && pattern[j] != *reinterpret_cast<char*>(&TextSectionBuffer[z + j]))
                            {
                                bFound = false;
                                break;
                            }
                        }

                        if (bFound)
                        {
                            MANUALMAP_LOG(
                                "A MMM Detected at 0x%llx with size %zu and memory type 0x%llx, base address: 0x%llX file "
                                "%s, has loaded: %d, has erased header: %d",
                                reinterpret_cast<DWORD64>(lpCurrentAddress), MemoryRegion.RegionSize, MemoryRegion.Type,
                                reinterpret_cast<DWORD64>(MemoryRegion.BaseAddress), szFileName, bHasLoaded, bHasErasedHeader);
                        }
                    }
                }
                else
                {
                    bytesRead = NULL;
                }*/
            }
        }

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        MANUALMAP_LOG("Manual Mapping Guard Pulse completed in %.5f seconds", fElapsedTime);

        std::this_thread::sleep_for(std::chrono::seconds(4));
    }
    _endthreadex(0);
}