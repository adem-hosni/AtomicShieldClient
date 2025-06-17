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

bool CManualMappingGuard::IsPEHeader(BYTE* Memory)
{
    __try
    {
        if (*((WORD*)Memory) != IMAGE_DOS_SIGNATURE)            // check for "MZ" at the start
            return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)Memory;
    IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)(Memory + pDosHeader->e_lfanew);

    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)            // check for "PE" signature
        return false;

    return true;
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

        auto MemoryMap = Utils::BuildModuledMemoryMap(g_pAtomicAntiCheat->GetProcessHandle());
        if (MemoryMap.size() == 0)
        {
            MANUALMAP_LOG("Failed to build module map! error: 0x%llx", GetLastError());
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

            if (MemoryRegion.RegionSize <= 4096)
                continue;

            if (!(MemoryRegion.Protect & (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_WRITECOPY)))
                continue;

            if (Utils::IsAddressInModuledRange(reinterpret_cast<DWORD64>(lpCurrentAddress), MemoryMap))
                continue;

            std::vector<BYTE> PEHeaderBuffer(512);
            if (!NT_SUCCESS(SysNtReadVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), MemoryRegion.BaseAddress, PEHeaderBuffer.data(), PEHeaderBuffer.size(),
                                                   &returnLength)) ||
                returnLength < sizeof(IMAGE_DOS_HEADER))
            {
                MANUALMAP_LOG("Failed to read memory at address 0x%llx, error: %d", reinterpret_cast<DWORD64>(lpCurrentAddress), GetLastError());
                continue;
            }

            if (IsPEHeader(PEHeaderBuffer.data()))
            {
                MANUALMAP_LOG("Found Suspicious PE header at address 0x%llx", reinterpret_cast<DWORD64>(lpCurrentAddress));
            }

            PSAPI_WORKING_SET_EX_INFORMATION workingSetInfo = {};
            workingSetInfo.VirtualAddress = MemoryRegion.BaseAddress;

            bool bFoundPossibleErasedHeaderModule = true;
            if (QueryWorkingSetEx(g_pAtomicAntiCheat->GetProcessHandle(), &workingSetInfo, sizeof(workingSetInfo)))
            {
                if (workingSetInfo.VirtualAttributes.Valid)
                {
                    if (!workingSetInfo.VirtualAttributes.Shared)
                    {
                        bool  bFoundPossibleSection = false;
                        BYTE BufferPossibleMappingSection[128]{0};

                        returnLength = NULL;
                        DWORD64 dwPossibleTextSectionAddress = (DWORD64)MemoryRegion.BaseAddress;
                        if (NT_SUCCESS(SysNtReadVirtualMemory(g_pAtomicAntiCheat->GetProcessHandle(), (PVOID)dwPossibleTextSectionAddress,
                                                              BufferPossibleMappingSection, sizeof(BufferPossibleMappingSection), &returnLength)) &&
                            returnLength > 0)
                        {
                            int iDetectedMappingSections = 0;
                            for (int i = 0; i < sizeof(BufferPossibleMappingSection) - 4; i++)
                            {
                                if (BufferPossibleMappingSection[i] != 0 && BufferPossibleMappingSection[i + 1] != 0 &&
                                    BufferPossibleMappingSection[i + 2] != 0 && BufferPossibleMappingSection[i + 3] != 0)
                                {
                                    bFoundPossibleSection = true;
                                    MANUALMAP_LOG("Found possible section at address 0x%llx >> Protection: 0x%x, State: 0x%x, Type: 0x%x, Size: %zu",
                                                  dwPossibleTextSectionAddress + i, MemoryRegion.Protect, MemoryRegion.State, MemoryRegion.Type,
                                                  MemoryRegion.RegionSize);
                                    iDetectedMappingSections++;
                                    break;
                                }
                            }
                            MANUALMAP_LOG("Found %d possible mapping sections", iDetectedMappingSections);

                        }
                        else
                        {
                            MANUALMAP_LOG("Failed to read memory at address 0x%llx, error: %d", dwPossibleTextSectionAddress, GetLastError());
                        }
                    }
                }
            }
            else
            {
                MANUALMAP_LOG("Failed to query working set for address 0x%llx, error: %d", reinterpret_cast<DWORD64>(MemoryRegion.BaseAddress), GetLastError());
                bFoundPossibleErasedHeaderModule = false;
            }
        }

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        MANUALMAP_LOG("Manual Mapping Guard Pulse completed in %.5f seconds", fElapsedTime);

        std::this_thread::sleep_for(std::chrono::seconds(4));
    }
    _endthreadex(0);
}