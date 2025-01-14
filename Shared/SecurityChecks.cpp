#include "SecurityChecks.h"
#include "SharedUtil.h"
#include <Psapi.h>
#include <intrin.h>
#include <processthreadsapi.h>
#pragma comment(lib, "ntdll.lib")

#define BUFFER_SIZE 0x1000
static LPVOID s_pGuardMem = nullptr;

typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation,            // Retrieves MEMORY_BASIC_INFORMATION
    MemoryWorkingSetInformation,
    MemoryMappedFilenameInformation,
    MemoryRegionInformation,
    MemoryWorkingSetExInformation
} MEMORY_INFORMATION_CLASS;

extern "C" NTSTATUS NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation,
                                         SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);


bool SecurityChecks::AntiBreakpoint::HasHardwareBreakpoint()
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (GetThreadContext(((HANDLE)-2), &ctx))
    {
        if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3 || ctx.Dr6 || ctx.Dr7)
        {
            return true;
        }
    }

    return false;
}

bool SecurityChecks::AntiBreakpoint::HasEntrypointBreakpoint()
{
    auto pIDH = reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleHandle(NULL));
    if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    auto pINH = reinterpret_cast<PIMAGE_NT_HEADERS>((reinterpret_cast<DWORD_PTR>(pIDH) + pIDH->e_lfanew));
    if (pINH->Signature != IMAGE_NT_SIGNATURE)
        return false;

    auto pEntryPoint = reinterpret_cast<PBYTE>((pINH->OptionalHeader.AddressOfEntryPoint + reinterpret_cast<DWORD_PTR>(pIDH)));
    return (pEntryPoint[0] == 0xCC);
}

bool SecurityChecks::AntiBreakpoint::HasMemoryBreakpoint()
{
    SYSTEM_INFO SystemInfo = {0};
    GetSystemInfo(&SystemInfo);

    auto pAllocation = VirtualAlloc(NULL, SystemInfo.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (pAllocation == NULL)
        return false;

    RtlFillMemory(pAllocation, 1, 0xC3);

    DWORD OldProtect = 0;
    if (VirtualProtect(pAllocation, SystemInfo.dwPageSize, PAGE_EXECUTE_READWRITE | PAGE_GUARD, &OldProtect) == 0)
        return false;

    __try
    {
        ((void (*)())pAllocation)();
    }
    __except (GetExceptionCode() == STATUS_GUARD_PAGE_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        VirtualFree(pAllocation, NULL, MEM_RELEASE);
        return false;
    }

    VirtualFree(pAllocation, NULL, MEM_RELEASE);
    return true;
}

bool SecurityChecks::AntiDebug::CheckCPUId()
{
    INT CPUInfo[4] = {-1};

    /* Query hypervisor precense using CPUID (EAX=1), BIT 31 in ECX */
    __cpuid(CPUInfo, 1);
    return ((CPUInfo[2] >> 31) & 1);
}


inline LPVOID CreateSafeMemoryPage(DWORD dwRegionSize, DWORD dwProtection)
{
    LPVOID pMemBase = nullptr;

    __try
    {
        pMemBase = VirtualAlloc(0, dwRegionSize, MEM_COMMIT | MEM_RESERVE, dwProtection);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return pMemBase;
}

bool SecurityChecks::AntiDump::InitializeAntiDump(HMODULE hModule)
{
    for (std::size_t i = 0; i < SharedUtil::GenerateRandomNumber(20, 50); i++)
        CreateSafeMemoryPage(BUFFER_SIZE, PAGE_READWRITE);

    s_pGuardMem = CreateSafeMemoryPage(BUFFER_SIZE, PAGE_READWRITE);

    for (std::size_t i = 0; i < SharedUtil::GenerateRandomNumber(20, 50); i++)
        CreateSafeMemoryPage(BUFFER_SIZE, PAGE_READWRITE);

    //	auto hTargetModule = g_winapiModuleTable->hBaseModule;
    auto hTargetModule = hModule;

    auto pDOS = (IMAGE_DOS_HEADER*)hTargetModule;
    if (pDOS->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    auto pINH = (IMAGE_NT_HEADERS*)(pDOS + pDOS->e_lfanew);
    //	if (pINH->Signature != IMAGE_NT_SIGNATURE)
    //		return false;

    auto pISH = (PIMAGE_SECTION_HEADER)(pINH + 1);
    if (!pISH)
        return false;

    auto dwOldProtect = 0UL;
    VirtualProtect((LPVOID)pISH, sizeof(LPVOID), PAGE_READWRITE, &dwOldProtect);

    pISH[0].VirtualAddress = reinterpret_cast<DWORD_PTR>(s_pGuardMem);

    VirtualProtect((LPVOID)pISH, sizeof(LPVOID), dwOldProtect, &dwOldProtect);

    return true;
}

bool SecurityChecks::AntiDump::IsDumpTriggered()
{
    if (!s_pGuardMem)
    {
        SharedUtil::AddDebugLog("Null guard ptr!");
        return true;
    }

    PSAPI_WORKING_SET_EX_INFORMATION pworkingSetExInformation = {s_pGuardMem, NULL};

    

    auto ntStatus = NtQueryVirtualMemory(((HANDLE)-1), NULL, MemoryWorkingSetExInformation, &pworkingSetExInformation,
                                                           sizeof(pworkingSetExInformation), NULL);
    if ((((NTSTATUS)(ntStatus)) >= 0))
    {
        if (pworkingSetExInformation.VirtualAttributes.Valid)
            return true;
    }

    return false;
}


#pragma optimize("", on)

