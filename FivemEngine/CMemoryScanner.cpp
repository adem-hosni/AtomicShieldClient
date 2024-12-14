#include "CMemoryScanner.h"
#include "SharedUtil.h"
#include <fstream>
#include <algorithm>
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

void CMemoryScanner::Attach(HANDLE hProcess)
{
    m_hProcess = hProcess;
}

std::string GetModuleFilenameFromAddress(HANDLE hProcess, const void* address)
{
    // Buffer to store module handles
    HMODULE modules[1024];
    DWORD   cbNeeded;

    // Retrieve all module handles in the target process
    if (EnumProcessModulesEx(hProcess, modules, sizeof(modules), &cbNeeded, LIST_MODULES_ALL))
    {
        size_t moduleCount = cbNeeded / sizeof(HMODULE);

        for (size_t i = 0; i < moduleCount; ++i)
        {
            MODULEINFO moduleInfo;
            if (GetModuleInformation(hProcess, modules[i], &moduleInfo, sizeof(moduleInfo)))
            {
                // Check if the address is within the module's memory range
                if (address >= moduleInfo.lpBaseOfDll && address < static_cast<void*>(static_cast<char*>(moduleInfo.lpBaseOfDll) + moduleInfo.SizeOfImage))
                {
                    SharedUtil::AddDebugLog("Yeeee");
                    // Get the module filename
                    char moduleName[MAX_PATH];
                    if (GetModuleFileNameExA(hProcess, modules[i], moduleName, MAX_PATH))
                    {
                        return std::string(moduleName);
                    }
                }
            }
        }
    }
    else
    {
        std::cerr << "EnumProcessModulesEx failed. Error: " << GetLastError() << std::endl;
    }

    return "<Not Found>";            // Address not found in any module
}

void CMemoryScanner::ScanStrings(std::map<std::string, std::vector<std::string>> Signatures)
{
    SYSTEM_INFO si;
    char*       currentmemorypage = 0;
    GetSystemInfo(&si);
    MEMORY_BASIC_INFORMATION info;

    if (!Signatures.size())
        return;

    if (!m_hProcess)
        return;

    SharedUtil::AddDebugLog("Begin Scan");
    std::string Signature = "api.tzproject.com";

    while (currentmemorypage < si.lpMaximumApplicationAddress)
    {
        NtQueryVirtualMemory(m_hProcess, currentmemorypage, MemoryBasicInformation, &info, sizeof(info), 0);

        if (info.State == MEM_COMMIT)
        {
            if (info.Protect == PAGE_READWRITE)
            {
                std::string buffer;
                buffer.resize(info.RegionSize + info.RegionSize / 2);            // so the buffer don"t overflow

                ZwReadVirtualMemory(m_hProcess, currentmemorypage, &buffer.at(0), info.RegionSize, 0);

                for (int begin = 0; begin < info.RegionSize; begin++)
                {
                    for (const auto& Item : Signatures)
                    {
                        std::string              SignatureTitle = Item.first;
                        std::vector<std::string> SignaturesList = Item.second;

                        for (auto Signature : SignaturesList)
                        {
                            if (buffer[begin] == Signature.at(0) && buffer[begin + Signature.length() - 1] == Signature.back())
                            {
                                std::string stringbuffer = buffer.substr(begin, Signature.length());

                                if (Signature.find(stringbuffer) != std::string::npos)
                                {
                                    char szModulePath[MAX_PATH];
                                    memset(szModulePath, 0, sizeof(szModulePath));
                                    HMODULE hModule = reinterpret_cast<HMODULE>(info.AllocationBase);
                                    if (!GetModuleFileName(hModule, szModulePath, sizeof(szModulePath)))
                                        sprintf(szModulePath, "<Failed to retreive module path 0x%x>", GetLastError());
                                    SharedUtil::AddDebugLog("Found %s at 0x%x in %s", Signature.c_str(), (uintptr_t)currentmemorypage + begin, szModulePath);
                                }
                            }
                        }
                    }
                }
            }
        }

        currentmemorypage += info.RegionSize;
    }
    SharedUtil::AddDebugLog("End Scan");
    currentmemorypage = 0;
}

void CMemoryScanner::AddSignatures(jsoncons::json Signatures)
{
    // m_Signatures.push_back(Signatures);
}
