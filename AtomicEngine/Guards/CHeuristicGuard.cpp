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
    static int  scanCycle = 0;
    static int  totalRegionsScanned = 0;
    static int  totalQueryFailures = 0;
    static int  consecutiveProblemCycles = 0;
    static bool wasVerboseLastCycle = false;

    scanCycle++;

    // Smart logging - verbose only for first cycles or when problems detected
    bool verboseLogging = (scanCycle <= 5) || (consecutiveProblemCycles > 0) || (scanCycle % 30 == 0);

    if (verboseLogging && !wasVerboseLastCycle)
    {
        SharedUtil::AddDebugLog("[Heuristic] === CYCLE %d STARTED (Verbose) ===", scanCycle);
    }

    constexpr SIZE_T kSampleSize = 256;

    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    RtlSecureZeroMemory(&objAttr, sizeof(objAttr));
    objAttr.Length = sizeof(objAttr);
    RtlSecureZeroMemory(&clientId, sizeof(clientId));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    if (verboseLogging)
    {
        SharedUtil::AddDebugLog("[Heuristic] Thread ID: %d, Priority: %d", GetThreadId(GetCurrentThread()), GetThreadPriority(GetCurrentThread()));
    }

    NTSTATUS status;

    while (g_pAtomicAntiCheat->RunScanners())
    {
        HANDLE hProcess = g_pAtomicAntiCheat->GetProcessHandle();

        if (verboseLogging)
        {
            SharedUtil::AddDebugLog("[Heuristic] Process handle: 0x%p, Valid: %s", hProcess, g_pAtomicAntiCheat->IsValidProcessHandle() ? "YES" : "NO");
        }

        while (!g_pAtomicAntiCheat->IsValidProcessHandle())
        {
            if (verboseLogging)
            {
                SharedUtil::AddDebugLog("[Heuristic] Waiting for valid process handle...");
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        g_pAtomicAntiCheat->GetNetwork()->Ping(eHeartbeatType::HEURISTIC_GUARD);

        // Only log heartbeat occasionally to reduce spam
        if (scanCycle % 10 == 0)
        {
            SharedUtil::AddDebugLog("[PING] Heuristic guard heartbeat sent (Cycle %d)", scanCycle);
        }

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        std::vector<RegionInfo>  regions;
        MEMORY_BASIC_INFORMATION MemoryRegion{};
        GetSystemInfo(&sysInfo);

        // Statistics for this scan cycle
        int currentCycleRegions = 0;
        int currentCycleQueryFails = 0;
        int currentCycleReadFails = 0;
        int currentCycleHashMatches = 0;
        int currentCycleSkippedRegions = 0;
        int currentCycleInvalidState = 0;
        int currentCycleInvalidProtect = 0;
        int currentCycleTooSmall = 0;

        if (verboseLogging)
        {
            SharedUtil::AddDebugLog("[Heuristic] Starting memory region enumeration...");
        }

        for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
             addr = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize)
        {
            PVOID  baseAddress = addr;
            SIZE_T rSize = sizeof(MemoryRegion);
            SIZE_T returnLength = 0;

            NTSTATUS queryStatus = SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &MemoryRegion, rSize, &returnLength);
            if (!NT_SUCCESS(queryStatus))
            {
                currentCycleQueryFails++;
                // Only log query failures in verbose mode or if they're becoming problematic
                if (verboseLogging && currentCycleQueryFails <= 3)
                {
                    SharedUtil::AddDebugLog("[Heuristic] QueryVirtualMemory FAILED - Addr: 0x%p, Status: 0x%X", baseAddress, queryStatus);
                }
                continue;
            }

            // Region filtering with detailed counters
            if (MemoryRegion.RegionSize < 400 * 1024)
            {
                currentCycleTooSmall++;
                continue;
            }

            if (MemoryRegion.State != MEM_COMMIT)
            {
                currentCycleInvalidState++;
                continue;
            }

            if (MemoryRegion.Type != MEM_PRIVATE)
            {
                continue;
            }

            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
            {
                currentCycleInvalidProtect++;
                continue;
            }

            if (!(MemoryRegion.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE)))
            {
                currentCycleInvalidProtect++;
                continue;
            }

            XXH64_hash_t currentHash = 0;
            BYTE         sampleBuffer[kSampleSize * 3] = {};
            size_t       totalCopied = 0;

            LPBYTE base = static_cast<LPBYTE>(MemoryRegion.BaseAddress);
            SIZE_T regionSize = MemoryRegion.RegionSize;
            LPVOID offsets[3] = {base, regionSize >= kSampleSize * 2 ? base + (regionSize / 2) : nullptr,
                                 regionSize >= kSampleSize * 3 ? base + (regionSize - kSampleSize) : nullptr};

            if (verboseLogging)
            {
                SharedUtil::AddDebugLog("[Heuristic] Scanning region - Base: 0x%p, Size: %zu, Protect: 0x%X", MemoryRegion.BaseAddress, regionSize,
                                        MemoryRegion.Protect);
            }

            for (int i = 0; i < 3; ++i)
            {
                if (!offsets[i])
                    continue;

                SIZE_T   r = 0;
                NTSTATUS readStatus = SysNtReadVirtualMemory(hProcess, offsets[i], sampleBuffer + totalCopied, kSampleSize, &r);
                if (NT_SUCCESS(readStatus) && r == kSampleSize)
                {
                    totalCopied += r;
                }
                else
                {
                    currentCycleReadFails++;
                    // Only log read failures in verbose mode or if they're frequent
                    if (verboseLogging || currentCycleReadFails > 5)
                    {
                        SharedUtil::AddDebugLog("[Heuristic] ReadVirtualMemory FAILED - Offset: %d, Addr: 0x%p, Status: 0x%X, BytesRead: %zu", i, offsets[i],
                                                readStatus, r);
                    }
                }
            }

            if (totalCopied == 0)
            {
                if (verboseLogging)
                {
                    SharedUtil::AddDebugLog("[Heuristic] No data read from region 0x%p, skipping", MemoryRegion.BaseAddress);
                }
                continue;
            }

            currentHash = XXH64(sampleBuffer, totalCopied, 0);

            if (verboseLogging)
            {
                SharedUtil::AddDebugLog("[Heuristic] Region 0x%p hash: 0x%llx, BytesRead: %zu", MemoryRegion.BaseAddress, currentHash, totalCopied);
            }

            bool regionAlreadyScanned = false;
            for (auto& entry : m_vScannedRegions)
            {
                if (entry.baseAddress == MemoryRegion.BaseAddress && entry.regionSize == MemoryRegion.RegionSize && entry.protect == MemoryRegion.Protect)
                {
                    if (entry.hash == currentHash)
                    {
                        regionAlreadyScanned = true;
                        currentCycleHashMatches++;
                    }
                    else
                    {
                        entry.hash = currentHash;
                    }
                    break;
                }
            }

            if (regionAlreadyScanned)
                continue;

            m_vScannedRegions.push_back({MemoryRegion.BaseAddress, MemoryRegion.RegionSize, MemoryRegion.Protect, currentHash});
            regions.push_back({MemoryRegion, baseAddress});
            currentCycleRegions++;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Update totals
        totalRegionsScanned += currentCycleRegions;
        totalQueryFailures += currentCycleQueryFails;

        // Determine if this cycle had problems
        bool cycleHasProblems = (currentCycleQueryFails > 10) || (currentCycleReadFails > 15) || (currentCycleRegions == 0);

        if (cycleHasProblems)
        {
            consecutiveProblemCycles++;
        }
        else
        {
            consecutiveProblemCycles = 0;
        }

        // Always log problem cycles, otherwise log based on frequency
        bool shouldLogDetails = cycleHasProblems || verboseLogging || (scanCycle % 15 == 0);

        if (shouldLogDetails)
        {
            SharedUtil::AddDebugLog("[Heuristic] Cycle %d - Regions: %d, QueryFails: %d, ReadFails: %d, HashMatches: %d", scanCycle, currentCycleRegions,
                                    currentCycleQueryFails, currentCycleReadFails, currentCycleHashMatches);

            if (cycleHasProblems)
            {
                SharedUtil::AddDebugLog("[Heuristic] PROBLEMS - QueryFails: %d, ReadFails: %d, Skipped: %d (Small: %d, State: %d, Protect: %d)",
                                        currentCycleQueryFails, currentCycleReadFails, currentCycleSkippedRegions, currentCycleTooSmall,
                                        currentCycleInvalidState, currentCycleInvalidProtect);
            }
        }

        auto scanFunc = [&](size_t startIdx, size_t endIdx)
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

            if (verboseLogging)
            {
                SharedUtil::AddDebugLog("[Heuristic] Signature scanning started - Range: %zu to %zu", startIdx, endIdx);
            }

            int currentCycleAllocFails = 0;
            int currentCycleSignatureReadFails = 0;
            int regionsProcessed = 0;

            for (size_t i = startIdx; i < endIdx && !m_bFound.load(); ++i)
            {
                regionsProcessed++;
                const auto& region = regions[i];
                SIZE_T      allocationSize = region.mbi.RegionSize;
                PVOID       buffer = nullptr;

                NTSTATUS allocStatus = SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!NT_SUCCESS(allocStatus))
                {
                    currentCycleAllocFails++;
                    // Only log allocation failures if they're becoming problematic
                    if (currentCycleAllocFails > 2 || verboseLogging)
                    {
                        SharedUtil::AddDebugLog("[Heuristic] Allocation FAILED - Region: 0x%p, Size: %zu, Status: 0x%X", region.mbi.BaseAddress, allocationSize,
                                                allocStatus);
                    }
                    continue;
                }

                SIZE_T   bytesRead = 0;
                NTSTATUS readStatus = SysNtReadVirtualMemory(hProcess, region.mbi.BaseAddress, buffer, allocationSize, &bytesRead);
                if (!NT_SUCCESS(readStatus) || bytesRead == 0)
                {
                    currentCycleSignatureReadFails++;
                    if (verboseLogging || currentCycleSignatureReadFails > 3)
                    {
                        SharedUtil::AddDebugLog("[Heuristic] Signature read FAILED - Region: 0x%p, Status: 0x%X, BytesRead: %zu", region.mbi.BaseAddress,
                                                readStatus, bytesRead);
                    }
                    SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
                    continue;
                }

                const char* dataPtr = reinterpret_cast<const char*>(buffer);
                bool        signatureFound = false;

                for (const auto& decryptedStr : m_vSignatures)
                {
                    if (decryptedStr.empty())
                        continue;

                    size_t foundPos = std::string_view(dataPtr, bytesRead).find(decryptedStr);
                    if (foundPos != std::string_view::npos)
                    {
                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(region.mbi.BaseAddress) + foundPos;
                        QueryPerformanceCounter(&end);

                        // ALWAYS log detections - this is critical
                        SharedUtil::AddDebugLog("[+] SIGNATURE DETECTED at 0x%llX in region 0x%llX", (DWORD64)lpFlaggedAddress,
                                                (DWORD64)region.mbi.BaseAddress);

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
                        signatureFound = true;
                        break;
                    }
                }

                SIZE_T   freeSize = allocationSize;
                NTSTATUS freeStatus = SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &freeSize, MEM_RELEASE);
                if (!NT_SUCCESS(freeStatus) && verboseLogging)
                {
                    SharedUtil::AddDebugLog("[Heuristic] WARNING: Failed to free buffer 0x%p, Status: 0x%X", buffer, freeStatus);
                }

                if (m_bFound.load())
                    break;
            }

            // Log signature scan issues only if they're significant
            if (currentCycleAllocFails > 2 || currentCycleSignatureReadFails > 5)
            {
                SharedUtil::AddDebugLog("[Heuristic] Scan issues - AllocFails: %d, SignatureReadFails: %d, Processed: %d/%zu", currentCycleAllocFails,
                                        currentCycleSignatureReadFails, regionsProcessed, (endIdx - startIdx));
            }
        };

        if (verboseLogging)
        {
            SharedUtil::AddDebugLog("[Heuristic] Starting signature scan on %zu regions...", regions.size());
        }

        scanFunc(0, regions.size());

        QueryPerformanceCounter(&end);
        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        // Smart completion logging
        bool shouldLogCompletion = cycleHasProblems || verboseLogging || (scanCycle % 20 == 0) || (fElapsedTime > 8.0f);

        if (shouldLogCompletion)
        {
            SharedUtil::AddDebugLog("[Heuristic] Cycle %d COMPLETE - Time: %.3fs, Regions: %d, QueryFails: %d, ReadFails: %d", scanCycle, fElapsedTime,
                                    currentCycleRegions, currentCycleQueryFails, currentCycleReadFails);
        }

        // Periodic summary (less frequent)
        if (scanCycle % 50 == 0)
        {
            SharedUtil::AddDebugLog("[Heuristic] SUMMARY - Cycles: %d, Total regions: %d, Total query fails: %d", scanCycle, totalRegionsScanned,
                                    totalQueryFailures);
        }

        m_tLastHeartbeat = time(NULL);

        // Clean up scanned regions cache periodically to prevent memory growth
        if (m_vScannedRegions.size() > 1000)
        {
            m_vScannedRegions.erase(m_vScannedRegions.begin(), m_vScannedRegions.begin() + 500);
            if (verboseLogging)
            {
                SharedUtil::AddDebugLog("[Heuristic] Cleaned scanned regions cache, now: %zu", m_vScannedRegions.size());
            }
        }

        wasVerboseLastCycle = verboseLogging;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SharedUtil::AddDebugLog("[Heuristic] Heuristic guard thread EXITING after %d cycles", scanCycle);
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