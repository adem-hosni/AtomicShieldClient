#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <algorithm>

CHeuristicGuard::CHeuristicGuard()
{
    m_bFound = false;
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
                    SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
                    continue;
                }

            const char* dataPtr = reinterpret_cast<const char*>(buffer);

            for (const auto& decryptedStr : m_vSignatures)
            {
                size_t foundPos = std::string_view(dataPtr, bytesRead).find(decryptedStr);
                if (foundPos != std::string_view::npos && !decryptedStr.empty())
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
                    m_bFound = true;
                    break;
                }
            }
            SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);

            if (m_bFound)
                break;
            // std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        }

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Scanned Regions: %d", fElapsedTime, iRegions);

        std::this_thread::sleep_for(std::chrono::seconds(25));
    }
    SysNtClose(hProcess);
}

_endthreadex(0);
}
#pragma optimize("", on)
