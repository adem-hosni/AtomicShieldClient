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
#pragma optimize("", off)
void CHeuristicGuard::DoPulse()
{
    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    RtlSecureZeroMemory(&objAttr, sizeof(objAttr));
    objAttr.Length = sizeof(objAttr);
    RtlSecureZeroMemory(&clientId, sizeof(clientId));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    HANDLE   hProcess;
    NTSTATUS status;

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
            std::this_thread::sleep_for(std::chrono::seconds(1));

        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(g_pAtomicAntiCheat->GetProcessID()));

        status = SysNtOpenProcess(&hProcess, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &objAttr, &clientId);
        if (!NT_SUCCESS(status))
            continue;

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        std::vector<RegionInfo>  regions;
        MEMORY_BASIC_INFORMATION MemoryRegion{};
        GetSystemInfo(&sysInfo);

        for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
             addr = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize)
        {
            PVOID  baseAddress = addr;
            SIZE_T regionSize = sizeof(MemoryRegion);
            SIZE_T returnLength = 0;

            if (!NT_SUCCESS(SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &MemoryRegion, regionSize, &returnLength)))
                continue;

            if (MemoryRegion.State != MEM_COMMIT || MemoryRegion.Type != MEM_PRIVATE)
                continue;

            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
                continue;

            if (!(MemoryRegion.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
                continue;

            regions.push_back({MemoryRegion, baseAddress});
        }

        // Determine number of threads based on CPU cores
        unsigned int numCores = std::thread::hardware_concurrency();
        unsigned int numThreads = 1;            // fallback to 1 thread if detection fails

        if (numCores >= 4)
            numThreads = 4;
        else if (numCores >= 2)
            numThreads = 2;
        else
            numThreads = 1;

        size_t quarter = regions.size() / numThreads;

        auto scanFunc = [&](size_t startIdx, size_t endIdx)
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

            for (size_t i = startIdx; i < endIdx && !m_bFound.load(); ++i)
            {
                if (regions[i].mbi.RegionSize > 25 * 1024 * 1024)            // Skip regions larger than 25MB
                    continue;

                const auto& region = regions[i];
                SIZE_T      allocationSize = region.mbi.RegionSize;
                PVOID       buffer = nullptr;

                if (!NT_SUCCESS(SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
                    continue;

                SIZE_T bytesRead = 0;
                if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, region.mbi.BaseAddress, buffer, allocationSize, &bytesRead)) || bytesRead == 0)
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
                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(region.mbi.BaseAddress) + foundPos;
                        QueryPerformanceCounter(&end);

                        float fScanTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

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
                if (m_bFound.load())
                    break;
            }
        };

        // Launch threads dynamically based on numThreads
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < numThreads; ++i)
        {
            size_t startIdx = i * quarter;
            size_t endIdx = (i == numThreads - 1) ? regions.size() : (i + 1) * quarter;
            threads.emplace_back(scanFunc, startIdx, endIdx);
        }

        for (auto& t : threads)
            t.join();

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Scanned Regions: %zu", fElapsedTime, regions.size());

        std::this_thread::sleep_for(std::chrono::seconds(45));
    }

    SysNtClose(hProcess);
    _endthreadex(0);
}
#pragma optimize("", on)
