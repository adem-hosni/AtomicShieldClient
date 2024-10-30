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

bool CMemoryScanner::DumpProcess()
{
    char szTempPath[MAX_PATH];
    memset(szTempPath, 0, sizeof(szTempPath));
    DWORD dwPathSize = GetTempPath(MAX_PATH, szTempPath);
    if (dwPathSize == NULL && dwPathSize > MAX_PATH)
        return false;

    memset(m_szLastDumpPath, 0, MAX_PATH);
    sprintf_s(m_szLastDumpPath, sizeof(m_szLastDumpPath), "%s\\%x%x", szTempPath, std::time(nullptr), GetTickCount());

    HANDLE hFile = CreateFile(m_szLastDumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(0, "Failed to create that shitty file!", 0, 0);
        return false;
    }
    HANDLE           hToken;
    TOKEN_PRIVILEGES tokenPrivileges;

    if (!SharedUtil::IsRunningAsAdministator())
    {
        
    }

    BOOL bSuccess = MiniDumpWriteDump(m_hProcess, m_dwProcessID, hFile, MiniDumpWithFullMemory, NULL, NULL, NULL);
    CloseHandle(hFile);

    return bSuccess;
}

std::vector<char> CMemoryScanner::LoadDumpBuffer()
{
    std::ifstream DumpFile(std::string(m_szLastDumpPath), std::ios::binary | std::ios::ate);
    if (!DumpFile.is_open())
        return {};

    std::vector<char> vBuffer;
    size_t sz = DumpFile.tellg();
    vBuffer.resize(sz);

    DumpFile.read(vBuffer.data(), sz);
    DumpFile.close();
    DeleteFile(m_szLastDumpPath);
    return vBuffer;
}

bool rpm(HANDLE hProcess, LPCVOID address, LPVOID buffer, SIZE_T size)
{
    if (hProcess == INVALID_HANDLE_VALUE)
        return false;

    SIZE_T bytesRead;
    if (!ReadProcessMemory(hProcess, address, buffer, size, &bytesRead))
    {
        return false;
    }
    return true;
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
    std::vector<char>        buffer;
    size_t                   bufferSize = 4096;            // Adjust as needed
    buffer.resize(bufferSize);
    
    while (address < sysInfo.lpMaximumApplicationAddress)
    {
        if (VirtualQueryEx(m_hProcess, address, &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            if (mbi.State == MEM_COMMIT && (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_READONLY))
            {
                for (LPVOID addr = address; addr < (LPBYTE)address + mbi.RegionSize; addr = (LPBYTE)addr + bufferSize)
                {
                    SIZE_T bytesToRead =
                        (LPBYTE)addr + bufferSize > (LPBYTE)address + mbi.RegionSize ? (SIZE_T)((LPBYTE)address + mbi.RegionSize - (LPBYTE)addr) : bufferSize;
                    if (rpm(m_hProcess, addr, buffer.data(), bytesToRead))
                    {
                        for (const auto& Item : Signatures)
                        {
                            std::string              SignatureTitle = Item.first;
                            std::vector<std::string> SignaturesList = Item.second;

                            for (const auto& Signature : SignaturesList)
                            {
                                auto it = std::find(m_vFoundSignatures.begin(), m_vFoundSignatures.end(), SignatureTitle);
                                if (it == m_vFoundSignatures.end())
                                {
                                    if (std::search(buffer.begin(), buffer.begin() + bytesToRead, Signature.begin(), Signature.end()) !=
                                        buffer.begin() + bytesToRead)
                                    {
                                        printf("Found %s at 0x%X\n", Signature.c_str(), addr);
                                        m_vFoundSignatures.push_back(SignatureTitle);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        address = (LPBYTE)address + mbi.RegionSize;
    }

    CloseHandle(m_hProcess);
}

void CMemoryScanner::debug(std::string printthatshit)
{
#ifdef debug
    std::cout << printthatshit << "\n";
#endif
}

void CMemoryScanner::AddSignatures(jsoncons::json Signatures)
{
    //m_Signatures.push_back(Signatures);
}