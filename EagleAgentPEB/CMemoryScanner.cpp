#include "CMemoryScanner.h"

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
    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessID);
}

void printError(const char* msg)
{
    DWORD eNum;
    char  sysMsg[256];
    char* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, eNum,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),            // Default language
                  sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do
    {
        *p-- = 0;
    } while ((p >= sysMsg) && ((*p == '.') || (*p < 33)));

    std::cerr << "\n  ERROR: " << msg << " failed with error " << eNum << " (" << sysMsg << ")";
}

bool readMemory(HANDLE hProcess, LPCVOID address, LPVOID buffer, SIZE_T size)
{
    SIZE_T bytesRead;
    if (!ReadProcessMemory(hProcess, address, buffer, size, &bytesRead))
    {
        printError("ReadProcessMemory");
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
                    if (readMemory(m_hProcess, addr, buffer.data(), bytesToRead))
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
                                        m_vFoundSignatures.push_back(Signature);
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