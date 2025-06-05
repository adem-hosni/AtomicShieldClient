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
    // One-time initialization for job object and process priority
    static std::once_flag initFlag;
    std::call_once(initFlag,
                   []
                   {
                       SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

                       HANDLE hJob = CreateJobObject(NULL, NULL);
                       if (hJob)
                       {
                           JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpuRateControl = {};
                           cpuRateControl.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
                           cpuRateControl.CpuRate = 500;            // 5% CPU cap

                           if (SetInformationJobObject(hJob, JobObjectCpuRateControlInformation, &cpuRateControl, sizeof(cpuRateControl)))
                           {
                               AssignProcessToJobObject(hJob, GetCurrentProcess());
                           }
                       }
                   });

    KernelCalls_OBJECT_ATTRIBUTES objAttr = {};
    KernelCalls_CLIENT_ID         clientId = {};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    ThreadPool            pool(std::thread::hardware_concurrency());
    static ULARGE_INTEGER lastIdleTime = {}, lastKernelTime = {}, lastUserTime = {};

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
            std::this_thread::sleep_for(std::chrono::seconds(1));

        HANDLE hProcess = NULL;
        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(g_pAtomicAntiCheat->GetProcessID()));
        NTSTATUS status = SysNtOpenProcess(&hProcess, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &objAttr, &clientId);
        if (!NT_SUCCESS(status))
            continue;

        m_bFound.store(false);

        LARGE_INTEGER frequency, start;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        // Calculate system CPU usage
        FILETIME idleTime, kernelTime, userTime;
        GetSystemTimes(&idleTime, &kernelTime, &userTime);
        ULARGE_INTEGER currentIdleTime = {.LowPart = idleTime.dwLowDateTime, .HighPart = idleTime.dwHighDateTime};
        ULARGE_INTEGER currentKernelTime = {.LowPart = kernelTime.dwLowDateTime, .HighPart = kernelTime.dwHighDateTime};
        ULARGE_INTEGER currentUserTime = {.LowPart = userTime.dwLowDateTime, .HighPart = userTime.dwHighDateTime};

        double cpuUsage = 0.0;
        if (lastIdleTime.QuadPart != 0)
        {
            ULONGLONG idleDiff = currentIdleTime.QuadPart - lastIdleTime.QuadPart;
            ULONGLONG kernelDiff = currentKernelTime.QuadPart - lastKernelTime.QuadPart;
            ULONGLONG userDiff = currentUserTime.QuadPart - lastUserTime.QuadPart;
            ULONGLONG totalSystem = kernelDiff + userDiff;

            if (totalSystem > 0)
                cpuUsage = (1.0 - (double)idleDiff / totalSystem) * 100.0;
        }
        lastIdleTime = currentIdleTime;
        lastKernelTime = currentKernelTime;
        lastUserTime = currentUserTime;

        // Determine thread count based on CPU load
        unsigned int numThreads = (cpuUsage > 80.0) ? 1 : (std::thread::hardware_concurrency() > 1) ? 2 : 1;

        // Collect memory regions
        std::vector<RegionInfo>  regions;
        MEMORY_BASIC_INFORMATION mbi = {};
        LPVOID                   addr = sysInfo.lpMinimumApplicationAddress;

        while (addr < sysInfo.lpMaximumApplicationAddress)
        {
            SIZE_T returnLength = 0;
            if (!NT_SUCCESS(SysNtQueryVirtualMemory(hProcess, addr, MemoryBasicInformation, &mbi, sizeof(mbi), &returnLength)))
            {
                addr = static_cast<LPBYTE>(addr) + sysInfo.dwPageSize;
                continue;
            }

            if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE)) &&
                (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
            {
                regions.push_back({mbi, addr});
            }
            addr = static_cast<LPBYTE>(mbi.BaseAddress) + mbi.RegionSize;
        }

        // Calculate max signature length for chunk overlap
        size_t maxSigLen = 0;
        for (const auto& sig : m_vSignatures)
        {
            if (sig.length() > maxSigLen)
                maxSigLen = sig.length();
        }
        const size_t overlap = maxSigLen > 0 ? maxSigLen - 1 : 0;
        const size_t CHUNK_SIZE = 256 * 1024;            // 256KB chunks

        // Scanning lambda with adaptive throttling
        auto scanFunc = [&](size_t startIdx, size_t endIdx)
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

            // Thread-local reusable buffer
            thread_local std::vector<char> buffer;
            if (buffer.capacity() < CHUNK_SIZE + overlap)
                buffer.reserve(CHUNK_SIZE + overlap);

            for (size_t i = startIdx; i < endIdx && !m_bFound.load(); ++i)
            {
                const RegionInfo& region = regions[i];
                const SIZE_T      regionSize = region.mbi.RegionSize;
                const DWORD_PTR   baseAddr = reinterpret_cast<DWORD_PTR>(region.mbi.BaseAddress);

                for (SIZE_T offset = 0; offset < regionSize && !m_bFound.load(); offset += CHUNK_SIZE)
                {
                    // Adaptive throttling based on system load
                    if (cpuUsage > 90.0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    else if (cpuUsage > 80.0)
                        std::this_thread::yield();

                    const SIZE_T chunkSize = min(CHUNK_SIZE + overlap, regionSize - offset);
                    SIZE_T       bytesRead = 0;

                    // Reuse buffer without reallocation
                    buffer.resize(chunkSize);
                    if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, reinterpret_cast<LPVOID>(baseAddr + offset), buffer.data(), chunkSize, &bytesRead)) ||
                        bytesRead == 0)
                    {
                        continue;
                    }

                    // String search in chunk
                    std::string_view chunkView(buffer.data(), bytesRead);
                    for (const auto& sig : m_vSignatures)
                    {
                        if (sig.empty() || m_bFound.load())
                            continue;

                        size_t pos = 0;
                        while ((pos = chunkView.find(sig, pos)) != std::string_view::npos)
                        {
                            LPVOID foundAddr = reinterpret_cast<LPVOID>(baseAddr + offset + pos);

                            SharedUtil::AddDebugLog("Found ya zebiiii");

                            m_bFound.store(true);
                            break;
                        }
                        if (m_bFound.load())
                            break;
                    }
                }
            }
        };

        // Parallel execution with completion tracking
        std::atomic<unsigned>   tasksRemaining{numThreads};
        std::mutex              cvMutex;
        std::condition_variable cv;
        const size_t            regionsPerThread = regions.size() / numThreads;

        for (unsigned i = 0; i < numThreads; ++i)
        {
            const size_t startIdx = i * regionsPerThread;
            const size_t endIdx = (i == numThreads - 1) ? regions.size() : startIdx + regionsPerThread;

            pool.enqueue(
                [=, &tasksRemaining, &cv]
                {
                    TaskCompletionGuard guard(tasksRemaining, cv);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20 * i));
                    scanFunc(startIdx, endIdx);
                });
        }

        // Wait for all tasks to complete
        std::unique_lock<std::mutex> lock(cvMutex);
        cv.wait(lock, [&] { return tasksRemaining == 0; });

        // Final timing and cleanup
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        float elapsed = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

        SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Regions: %zu | CPU Load: %.1f%%", elapsed, regions.size(), cpuUsage);

        std::this_thread::sleep_for(std::chrono::seconds(cpuUsage > 80.0 ? 90 : 45));
    }
    _endthreadex(0);
}
#pragma optimize("", on)