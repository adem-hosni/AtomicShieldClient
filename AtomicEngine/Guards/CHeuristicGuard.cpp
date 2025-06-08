#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <algorithm>
#include <ThreadPool.cpp>

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

struct RegionInfo
{
    MEMORY_BASIC_INFORMATION mbi;
    LPVOID                   baseAddress;
};

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

float GetCPUUsageForProcess(HANDLE hProcess, int intervalMs = 1000)
{
    FILETIME ftCreate1, ftExit1, ftKernel1, ftUser1;
    FILETIME ftCreate2, ftExit2, ftKernel2, ftUser2;

    if (!GetProcessTimes(hProcess, &ftCreate1, &ftExit1, &ftKernel1, &ftUser1))
        return -1.0f;

    ULARGE_INTEGER k1, u1;
    k1.LowPart = ftKernel1.dwLowDateTime;
    k1.HighPart = ftKernel1.dwHighDateTime;

    u1.LowPart = ftUser1.dwLowDateTime;
    u1.HighPart = ftUser1.dwHighDateTime;

    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));

    if (!GetProcessTimes(hProcess, &ftCreate2, &ftExit2, &ftKernel2, &ftUser2))
        return -1.0f;

    ULARGE_INTEGER k2, u2;
    k2.LowPart = ftKernel2.dwLowDateTime;
    k2.HighPart = ftKernel2.dwHighDateTime;

    u2.LowPart = ftUser2.dwLowDateTime;
    u2.HighPart = ftUser2.dwHighDateTime;

    // Calculate CPU time used during interval
    ULONGLONG kernelTime = k2.QuadPart - k1.QuadPart;
    ULONGLONG userTime = u2.QuadPart - u1.QuadPart;
    ULONGLONG totalTime100ns = kernelTime + userTime;

    // Convert to seconds
    float secondsPassed = intervalMs / 1000.0f;
    float cpuSecondsUsed = totalTime100ns / 10000000.0f;            // Convert from 100ns to seconds

    // Logical cores
    int numCores = std::thread::hardware_concurrency();

    // Calculate CPU usage %
    float cpuUsage = (cpuSecondsUsed / (secondsPassed * numCores)) * 100.0f;
    return cpuUsage;
}

#pragma optimize("", off)
void CHeuristicGuard::DoPulse()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    auto MemoryMap = Utils::BuildModuledMemoryMap(g_pAtomicAntiCheat->GetProcessHandle());

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (!g_pAtomicAntiCheat->IsValidProcessHandle())
            std::this_thread::sleep_for(std::chrono::seconds(1));

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        std::vector<RegionInfo>  regions;
        MEMORY_BASIC_INFORMATION MemoryRegion{};
        HANDLE                   hProcess = g_pAtomicAntiCheat->GetProcessHandle();

        // Memory region collection
        for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
             addr = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize)
        {
            PVOID  baseAddress = addr;
            SIZE_T regionSize = sizeof(MemoryRegion);
            SIZE_T returnLength = 0;

            if (!NT_SUCCESS(SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &MemoryRegion, regionSize, &returnLength)))
                continue;

            // Filter conditions
            if (MemoryRegion.State != MEM_COMMIT)
                continue;
            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
                continue;
            if (!(MemoryRegion.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
                continue;
            if (MemoryRegion.RegionSize < 512 * 1024)
                continue;
            if (Utils::IsAddressInModuledRange(reinterpret_cast<DWORD64>(addr), MemoryMap))
                continue;

            regions.push_back({MemoryRegion, baseAddress});
        }

        // Scanning with CPU throttling
        for (size_t i = 0; i < regions.size() && !m_bFound.load(); i++)
        {
            const auto& region = regions[i];

            // CPU throttling - sleep every 50 regions when scanning many regions
            if (regions.size() > 100 && i % 50 == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            SIZE_T allocationSize = region.mbi.RegionSize;
            PVOID  buffer = nullptr;

            if (!NT_SUCCESS(SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
                continue;

            SIZE_T bytesRead = 0;
            if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, region.mbi.BaseAddress, buffer, allocationSize, &bytesRead)) || bytesRead == 0)
            {
                SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
                continue;
            }

            // String search
            std::string_view dataView(reinterpret_cast<const char*>(buffer), bytesRead);
            for (const auto& decryptedStr : m_vSignatures)
            {
                if (dataView.find(decryptedStr) != std::string_view::npos && !decryptedStr.empty())
                {
                    // Detection handling (same as before)
                    LPVOID lpFlaggedAddress = static_cast<LPBYTE>(region.mbi.BaseAddress) + dataView.find(decryptedStr);

                    QueryPerformanceCounter(&end);
                    float fScanTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

                    SharedUtil::AddDebugLog("[+] Found signature at address 0x%llX in region 0x%llX (size: %zu bytes) with protection 0x%llX",
                                            (DWORD64)lpFlaggedAddress, (DWORD64)region.mbi.BaseAddress, region.mbi.RegionSize, region.mbi.Protect);

                    g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", decryptedStr},
                                                                                {"memory_address", (DWORD64)lpFlaggedAddress},
                                                                                {"region_size", region.mbi.RegionSize},
                                                                                {"base_address", (DWORD64)region.mbi.BaseAddress},
                                                                                {"region_type", (DWORD64)region.mbi.Type},
                                                                                {"region_state", (DWORD64)region.mbi.State},
                                                                                {"region_protect", (DWORD64)region.mbi.Protect},
                                                                                {"allocation_protect", (DWORD64)region.mbi.AllocationProtect},
                                                                                {"allocation_address", (DWORD64)region.mbi.AllocationBase},
                                                                                {"scan_time", std::to_string(fScanTime) + "s"}});

                    g_pAtomicAntiCheat->RunScanners(false);
                    m_bFound.store(true);
                    break;
                }
            }

            SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
        }

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Scanned Regions: %zu", fElapsedTime, regions.size());

        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    _endthreadex(0);
}
#pragma optimize("", on)