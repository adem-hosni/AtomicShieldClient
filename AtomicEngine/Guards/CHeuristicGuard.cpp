#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <algorithm>
#include <ThreadPool.cpp>
#pragma comment(lib, "Dnsapi.lib")
#include <windns.h>



typedef struct _DNSCACHEENTRY
{
    struct _DNSCACHEENTRY* pNext;
    PWSTR                  pszName;
    WORD                   wType;
    WORD                   wDataLength;
    DWORD                  dwFlags;
} DNSCACHEENTRY, *PDNSCACHEENTRY;

typedef DWORD(WINAPI* DNS_GET_CACHE_DATA_TABLE)(PDNSCACHEENTRY);
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

DNS_RECORD* GetDNS(const std::wstring& domain)
{
    PDNS_RECORD pRecord = nullptr;
    DNS_STATUS  status = DnsQuery_W(domain.c_str(), DNS_TYPE_A, DNS_QUERY_STANDARD, nullptr, &pRecord, nullptr);

    if (status != ERROR_SUCCESS)
    {
        SharedUtil::AddDebugLog("[-] DnsQuery_W failed for %ws with status %d", domain.c_str(), status);
        return nullptr;
    }

    PDNS_RECORD pIter = pRecord;

    while (pIter)
    {
        if (pIter->wType == DNS_TYPE_A)            // IPv4
        {
            CHAR    ipBuffer[INET_ADDRSTRLEN];
            IN_ADDR ipAddr;
            ipAddr.S_un.S_addr = pIter->Data.A.IpAddress;
            inet_ntop(AF_INET, &ipAddr, ipBuffer, sizeof(ipBuffer));

            SharedUtil::AddDebugLog("[+] DNS A record for %ws: %s", domain.c_str(), ipBuffer);

            // Return the first A record
            return pIter;
        }

        pIter = pIter->pNext;
    }

    SharedUtil::AddDebugLog("[-] No A record found for %ws", domain.c_str());

    if (pRecord)
        DnsRecordListFree(pRecord, DnsFreeRecordList);

    return nullptr;
}

//#pragma optimize("", off)
//void CHeuristicGuard::DoPulse()
//{
//    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
//    KernelCalls_CLIENT_ID         clientId{};
//    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
//
//    RtlSecureZeroMemory(&objAttr, sizeof(objAttr));
//    objAttr.Length = sizeof(objAttr);
//    RtlSecureZeroMemory(&clientId, sizeof(clientId));
//
//    SYSTEM_INFO sysInfo;
//    GetSystemInfo(&sysInfo);
//
//    HANDLE     hProcess;
//    NTSTATUS   status;
//    ThreadPool pool(std::thread::hardware_concurrency());            // create thread pool
//
//
//   auto MemoryMap = Utils::BuildModuledMemoryMap(g_pAtomicAntiCheat->GetProcessHandle());
//    if (MemoryMap.empty())
//    {
//        SharedUtil::AddDebugLog("[-] Memory map is empty");
//        SysNtClose(hProcess);
//        std::this_thread::sleep_for(std::chrono::seconds(5));
//    }
//    while (g_pAtomicAntiCheat->RunScanners())
//    {
//        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
//            std::this_thread::sleep_for(std::chrono::seconds(1));
//
//        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(g_pAtomicAntiCheat->GetProcessID()));
//
//        status = SysNtOpenProcess(&hProcess, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &objAttr, &clientId);
//        if (!NT_SUCCESS(status))
//            continue;
//
//        LARGE_INTEGER frequency, start, end;
//        QueryPerformanceFrequency(&frequency);
//        QueryPerformanceCounter(&start);
//
//        std::vector<RegionInfo>  regions;
//        MEMORY_BASIC_INFORMATION MemoryRegion{};
//        GetSystemInfo(&sysInfo);
//
//        for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
//             addr = static_cast<LPBYTE>(MemoryRegion.BaseAddress) + MemoryRegion.RegionSize)
//        {
//            PVOID  baseAddress = addr;
//            SIZE_T regionSize = sizeof(MemoryRegion);
//            SIZE_T returnLength = 0;
//
//            if (!NT_SUCCESS(SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &MemoryRegion, regionSize, &returnLength)))
//                continue;
//
//            if (MemoryRegion.State != MEM_COMMIT || MemoryRegion.Type != MEM_PRIVATE)
//                continue;
//
//            if (MemoryRegion.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_WRITECOMBINE))
//                continue;
//
//            if (!(MemoryRegion.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
//                continue;
//
//            //if (Utils::IsAddressInModuledRange(reinterpret_cast<DWORD64>(addr), MemoryMap))
//            //    continue;
//
//            if (MemoryRegion.RegionSize > 50 * 1024 * 1024)
//                continue;
//
//            regions.push_back({MemoryRegion, baseAddress});
//        }
//        unsigned int numCores = std::thread::hardware_concurrency();
//        unsigned int numThreads = 1;
//
//        if (numCores >= 2)
//            numThreads = 2;
//        else
//            numThreads = 1;
//
//        size_t quarter = regions.size() / numThreads;
//
//        auto scanFunc = [&](size_t startIdx, size_t endIdx)
//        {
//            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
//
//            for (size_t i = startIdx; i < endIdx && !m_bFound.load(); ++i)
//            {
//                const auto& region = regions[i];
//                SIZE_T      allocationSize = region.mbi.RegionSize;
//                PVOID       buffer = nullptr;
//
//                if (!NT_SUCCESS(SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
//                    continue;
//
//                SIZE_T bytesRead = 0;
//                if (!NT_SUCCESS(SysNtReadVirtualMemory(hProcess, region.mbi.BaseAddress, buffer, allocationSize, &bytesRead)) || bytesRead == 0)
//                {
//                    SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
//                    continue;
//                }
//
//                const char* dataPtr = reinterpret_cast<const char*>(buffer);
//
//                for (const auto& decryptedStr : m_vSignatures)
//                {
//                    size_t foundPos = std::string_view(dataPtr, bytesRead).find(decryptedStr);
//                    if (foundPos != std::string_view::npos && !decryptedStr.empty())
//                    {
//                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(region.mbi.BaseAddress) + foundPos;
//                        QueryPerformanceCounter(&end);
//                        SharedUtil::AddDebugLog("[+] Found signature at address 0x%llX in region 0x%llX (size: %zu bytes) with protection 0x%llX",
//                                                (DWORD64)lpFlaggedAddress, (DWORD64)region.mbi.BaseAddress, region.mbi.RegionSize,
//                                                region.mbi.Protect);
//
//                        float fScanTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
//
//                        g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", decryptedStr},
//                                                                                    {"memory_address", (DWORD64)lpFlaggedAddress},
//                                                                                    {"region_size", region.mbi.RegionSize},
//                                                                                    {"base_address", (DWORD64)region.mbi.BaseAddress},
//                                                                                    {"region_type", (DWORD64)region.mbi.Type},
//                                                                                    {"region_state", (DWORD64)region.mbi.State},
//                                                                                    {"region_protect", (DWORD64)region.mbi.Protect},
//                                                                                    {"allocation_protect", (DWORD64)region.mbi.AllocationProtect},
//                                                                                    {"allocation_address", (DWORD64)region.mbi.AllocationBase},
//                                                                                    {"scan_time", std::to_string(fScanTime) + "s"}});
//
//                        g_pAtomicAntiCheat->RunScanners(false);
//                        m_bFound.store(true);
//                        break;
//                    }
//                }
//
//                SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
//                if (m_bFound.load())
//                    break;
//            }
//        };
//
//        for (unsigned int i = 0; i < numThreads; ++i)
//        {
//            size_t startIdx = i * quarter;
//            size_t endIdx = (i == numThreads - 1) ? regions.size() : (i + 1) * quarter;
//
//            pool.enqueue(
//                [=]()
//                {
//                    std::this_thread::sleep_for(std::chrono::milliseconds(20 * i));            // staggered start
//                    scanFunc(startIdx, endIdx);
//                });
//        }
//
//        QueryPerformanceCounter(&end);
//        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
//
//        SharedUtil::AddDebugLog("[+] Scan completed in %.5fs | Scanned Regions: %zu", fElapsedTime, regions.size());
//
//        std::this_thread::sleep_for(std::chrono::seconds(90));
//    }
//
//    SysNtClose(hProcess);
//    _endthreadex(0);
//}
//#pragma optimize("", on)


void CHeuristicGuard::DoPulse()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        for (const auto& decryptedStr : m_vSignatures)
        {
            std::wstring targetDomain = std::wstring(decryptedStr.begin(), decryptedStr.end());           
            SharedUtil::AddDebugLog("[*] Checking DNS cache for domain: %ws", targetDomain.c_str());

            DNS_RECORD* dns_record = GetDNS(targetDomain);

            if (dns_record)
            {
                SharedUtil::AddDebugLog("[+] DNS record found: %s -> (type: %d)", targetDomain.c_str(),  dns_record->wType);
                g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", decryptedStr},
                                                                            {"memory_address", ""},
                                                                            {"region_size", ""},
                                                                            {"base_address", ""},
                                                                            {"region_type", ""},
                                                                            {"region_state", ""},
                                                                            {"region_protect", ""},
                                                                            {"allocation_protect", ""},
                                                                            {"allocation_address", ""},
                                                                            {"scan_time", "0s"}});
                g_pAtomicAntiCheat->RunScanners(false);
                m_bFound.store(true);
                break;
            }
            else
            {
                SharedUtil::AddDebugLog("[-] DNS record for %ws was not found.", targetDomain.c_str());
            }

        }

        if (m_bFound.load())
            break;

        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    _endthreadex(0);
}
