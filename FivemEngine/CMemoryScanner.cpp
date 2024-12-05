#include "CMemoryScanner.h"
#include "SharedUtil.h"
#include <fstream>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

CMemoryScanner* g_pMemoryScanner = new CMemoryScanner();

CMemoryScanner::CMemoryScanner() : m_uiLatestScanResult(0)
{
}

CMemoryScanner::~CMemoryScanner()
{
    CloseHandle(m_hProcess);
}

void CMemoryScanner::Attach(DWORD dwProcessID)
{
    GetSystemInfo(&m_SystemInfo);
    m_dwProcessID = dwProcessID;
    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessID);
}

void CMemoryScanner::ScanMemoryRegion(HANDLE hProcess, LPVOID start, LPVOID end, size_t bufferSize)
{
    SharedUtil::AddDebugLog("Begin Scan");
    MEMORY_BASIC_INFORMATION mbi;
    std::vector<char>        buffer(bufferSize);

    for (LPVOID address = start; address < end;)
    {
        if (VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READWRITE | PAGE_READONLY)))
            {
                for (LPVOID addr = address; addr < (LPBYTE)address + mbi.RegionSize; addr = (LPBYTE)addr + bufferSize)
                {
                    SIZE_T bytesRead;
                    SIZE_T bytesToRead = min((SIZE_T)((LPBYTE)address + mbi.RegionSize - (LPBYTE)addr), bufferSize);

                    if (ReadProcessMemory(hProcess, addr, buffer.data(), bytesToRead, &bytesRead))
                    {
                        // for (const auto& Signature : vSections)
                        //{
                        //     if (std::search(buffer.begin(), buffer.begin() + bytesToRead, Signature.begin(), Signature.end()) != buffer.begin() +
                        //     bytesToRead)
                        //     {
                        //         //std::lock_guard<std::mutex> lock(logMutex);
                        //         AddDebugLog("Section %s found at 0x%p", Signature.c_str(), addr);
                        //     }
                        // }
                    }
                }
            }
        }
        address = (LPBYTE)address + mbi.RegionSize;
    }

    SharedUtil::AddDebugLog("End Scan");
}

void CMemoryScanner::ScanStrings(std::map<std::string, std::vector<std::string>> Signatures)
{
    if (!Signatures.size())
        return;

    if (!m_hProcess)
        return;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION mbi;
    LPVOID                   address = sysInfo.lpMinimumApplicationAddress;
    size_t                   bufferSize = 65536;            // Larger buffer size

    LPVOID totalRange = (LPVOID)((DWORD)sysInfo.lpMaximumApplicationAddress - (DWORD)sysInfo.lpMinimumApplicationAddress);
    size_t numThreads = std::thread::hardware_concurrency() * 2;            // Get available cores
    size_t chunkSize = (DWORD)totalRange / numThreads;

    std::vector<std::thread> threads;

    for (size_t i = 0; i < numThreads; ++i)
    {
        LPVOID start = (LPBYTE)sysInfo.lpMinimumApplicationAddress + i * chunkSize;
        LPVOID end = (i == numThreads - 1) ? sysInfo.lpMaximumApplicationAddress : (LPBYTE)start + chunkSize;

        threads.emplace_back(ScanMemoryRegion, m_hProcess, start, end, bufferSize);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

void CMemoryScanner::AddSignatures(jsoncons::json Signatures)
{
    //m_Signatures.push_back(Signatures);
}