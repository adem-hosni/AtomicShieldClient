#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <algorithm>

CHeuristicGuard::CHeuristicGuard()
{
    m_bFound = false;
    m_tLastHeartbeat = NULL;
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
    constexpr SIZE_T kSampleSize = 256;

    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    RtlSecureZeroMemory(&objAttr, sizeof(objAttr));
    objAttr.Length = sizeof(objAttr);
    RtlSecureZeroMemory(&clientId, sizeof(clientId));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    NTSTATUS status;

    while (g_pAtomicAntiCheat->RunScanners())
    {
        HANDLE hProcess = g_pAtomicAntiCheat->GetProcessHandle();
        while (!g_pAtomicAntiCheat->IsValidProcessHandle())
            std::this_thread::sleep_for(std::chrono::seconds(1));

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
            SIZE_T rSize = sizeof(MemoryRegion);
            SIZE_T returnLength = 0;

            if (!NT_SUCCESS(SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &MemoryRegion, rSize, &returnLength)))
            {
                //SharedUtil::AddDebugLog("Failed to query vm at 0x%x", baseAddress);
                continue;
            }

            if (MemoryRegion.RegionSize < 400 * 1024)
                continue;

            if (MemoryRegion.State != MEM_COMMIT || MemoryRegion.Type != MEM_PRIVATE)
                continue;

            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
                continue;

            if (!(MemoryRegion.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE)))
                continue;

            XXH64_hash_t currentHash = 0;
            BYTE         sampleBuffer[kSampleSize * 3] = {};
            size_t       totalCopied = 0;

            LPBYTE base = static_cast<LPBYTE>(MemoryRegion.BaseAddress);
            SIZE_T regionSize = MemoryRegion.RegionSize;
            LPVOID offsets[3] = {base, regionSize >= kSampleSize * 2 ? base + (regionSize / 2) : nullptr,
                                 regionSize >= kSampleSize * 3 ? base + (regionSize - kSampleSize) : nullptr};

            for (int i = 0; i < 3; ++i)
            {
                if (!offsets[i])
                    continue;

                SIZE_T r = 0;
                if (NT_SUCCESS(SysNtReadVirtualMemory(hProcess, offsets[i], sampleBuffer + totalCopied, kSampleSize, &r)) && r == kSampleSize)
                    totalCopied += r;
            }

            if (totalCopied == 0)
                continue;

            currentHash = XXH64(sampleBuffer, totalCopied, 0);

            bool regionAlreadyScanned = false;
            for (auto& entry : m_vScannedRegions)
            {
                if (entry.baseAddress == MemoryRegion.BaseAddress && entry.regionSize == MemoryRegion.RegionSize && entry.protect == MemoryRegion.Protect)
                {
                    if (entry.hash == currentHash)
                        regionAlreadyScanned = true;
                    else
                        entry.hash = currentHash;
                    break;
                }
            }

            if (regionAlreadyScanned)
                continue;

            m_vScannedRegions.push_back({MemoryRegion.BaseAddress, MemoryRegion.RegionSize, MemoryRegion.Protect, currentHash});
            regions.push_back({MemoryRegion, baseAddress});
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto scanFunc = [&](size_t startIdx, size_t endIdx)
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

            for (size_t i = startIdx; i < endIdx && !m_bFound.load(); ++i)
            {
                const auto& region = regions[i];
                SIZE_T      allocationSize = region.mbi.RegionSize;
                PVOID       buffer = nullptr;

                if (!NT_SUCCESS(SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
                {
                    SharedUtil::AddDebugLog("Failed to allocate vm in 0x%p", buffer);
                    continue;
                }

                SIZE_T bytesRead = 0;
                if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, region.mbi.BaseAddress, buffer, allocationSize, &bytesRead)) || bytesRead == 0)
                {
                    SharedUtil::AddDebugLog("Failed to read vm in 0x%x with size 0x%x", region.mbi.BaseAddress, allocationSize);
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
                        SharedUtil::AddDebugLog("[+] Found signature at address 0x%llX in region 0x%llX (size: %zu bytes) with protection 0x%llX",
                                                (DWORD64)lpFlaggedAddress, (DWORD64)region.mbi.BaseAddress, region.mbi.RegionSize, region.mbi.Protect);

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

        scanFunc(0, regions.size());

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Scanned Regions: %zu", fElapsedTime, regions.size());
        m_tLastHeartbeat = time(NULL);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    _endthreadex(0);
}
#pragma optimize("", on)

void CHeuristicGuard::ClearDetections()
{
    m_bFound.store(false);
    m_vScannedRegions.clear();
    m_vSignatures.clear();
    m_tLastHeartbeat = NULL;
}