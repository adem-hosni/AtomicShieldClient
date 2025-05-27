#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <algorithm>

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

void CHeuristicGuard::Initialize()
{
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_vSignatures.push_back(Utils::CaesarDecrypt(Signature, 3));
        }
    }
}

#pragma optimize("", off)
void CHeuristicGuard::DoPulse()
{
    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
    objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
    RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    HANDLE   hProcess;
    NTSTATUS status;

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(g_pAtomicAntiCheat->GetProcessID()));

        status = SysNtOpenProcess(&hProcess, (0x0400) | (0x0010), &objAttr, &clientId);
        if (!NT_SUCCESS(status))
            continue;

        {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            MEMORY_BASIC_INFORMATION MemoryRegion{};
            bool                     found = false;
            int                      iRegions = 0;

            for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
                 addr = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize)
            {
                iRegions++;
                PVOID  baseAddress = addr;
                SIZE_T regionSize = sizeof(MemoryRegion);
                SIZE_T returnLength = 0;
                status = SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &MemoryRegion, regionSize, &returnLength);
                if (!NT_SUCCESS(status) || MemoryRegion.State != MEM_COMMIT || MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE) ||
                    !(MemoryRegion.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) || MemoryRegion.Type != MEM_PRIVATE)
                    continue;

                SIZE_T allocationSize = MemoryRegion.RegionSize;

                PVOID buffer = nullptr;
                status = SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!NT_SUCCESS(status) || buffer == nullptr)
                {
                    buffer = nullptr;
                    continue;
                }

                SIZE_T bytesRead = 0;
                status = SysNtReadVirtualMemory(hProcess, MemoryRegion.BaseAddress, buffer, allocationSize, &bytesRead);
                if (!NT_SUCCESS(status) || bytesRead == 0)
                {
                    if (status == 0x8000000D)
                    {
                        status = SysNtReadVirtualMemory(hProcess, MemoryRegion.BaseAddress, buffer, allocationSize, &bytesRead);
                        if (!NT_SUCCESS(status))
                        {
                            SharedUtil::AddDebugLog(
                                "[-] Failed to read memory at 0x%p region size: %d (Error 0x%x, status: 0x%016llX, Protection Flags: 0x%llx, Memory State: "
                                "0x%x, bytes "
                                "read: %d)",
                                MemoryRegion.BaseAddress, MemoryRegion.RegionSize, GetLastError(), status, MemoryRegion.Protect, MemoryRegion.State, bytesRead);
                            SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);

                            if (bytesRead == 0)
                            {
                                addr = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize;
                                continue;
                            }
                        }
                    }
                }

                const char* dataPtr = reinterpret_cast<const char*>(buffer);

                for (const auto& decryptedStr : m_vSignatures)
                {
                    size_t foundPos = std::string_view(dataPtr, bytesRead).find(decryptedStr);
                    if (foundPos != std::string_view::npos && !decryptedStr.empty() && decryptedStr.find_first_not_of(" \t\n\r\0") != std::string::npos)
                    {
                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + foundPos;
                        SharedUtil::AddDebugLog("Found at 0x%p", lpFlaggedAddress);

                        QueryPerformanceCounter(&end);
                        float fScanTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

                        g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", decryptedStr},
                                                                                    {"memory_address", (DWORD64)lpFlaggedAddress},
                                                                                    {"region_size", MemoryRegion.RegionSize},
                                                                                    {"base_address", (DWORD64)MemoryRegion.BaseAddress},
                                                                                    {"region_type", (DWORD64)MemoryRegion.Type},
                                                                                    {"region_state", (DWORD64)MemoryRegion.State},
                                                                                    {"region_protect", (DWORD64)MemoryRegion.Protect},
                                                                                    {"allocation_protect", (DWORD64)MemoryRegion.AllocationProtect},
                                                                                    {"allocation_address", (DWORD64)MemoryRegion.AllocationBase},
                                                                                    {"scan_time", std::to_string(fScanTime) + "s"}});

                        g_pAtomicAntiCheat->RunScanners(false);
                        break;
                        break;
                    }
                }
                SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
            }

            QueryPerformanceCounter(&end);
            float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

            SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Scanned Regions: %d", fElapsedTime, iRegions);

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        SysNtClose(hProcess);
    }

    _endthreadex(0);
}
#pragma optimize("", on)
