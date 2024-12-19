#include "StdInc.h"

CHeuristicGuard::CHeuristicGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
    m_dwCurrentAddress = NULL;
    m_dwMaxAddress = NULL;
    m_Signatures = {};
}

CHeuristicGuard::~CHeuristicGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        if (m_Signatures.count(name))
            m_Signatures[name].insert(m_Signatures[name].end(), std::make_move_iterator(vector.begin()), std::make_move_iterator(vector.end()));
        else
            m_Signatures[name] = std::move(vector);
    }
    SharedUtil::AddDebugLog("Sigs: %d", m_Signatures.size());
}

void CHeuristicGuard::DoPulse()
{
    char* currentmemorypage = 0;
    SharedUtil::AddDebugLog("Begin Scan");
    std::string Signature = "api.tzproject.com";

    SharedUtil::AddDebugLog("m_Sig: %d", m_Signatures.size());

    while (currentmemorypage < m_systemInfo.lpMaximumApplicationAddress)
    {
        NtQueryVirtualMemory(GetCurrentProcess(), currentmemorypage, MemoryBasicInformation, &m_mbi, sizeof(m_mbi), 0);

        if (m_mbi.State == MEM_COMMIT)
        {
            if (m_mbi.Protect == PAGE_READWRITE)
            {
                std::string buffer;
                buffer.resize(m_mbi.RegionSize + m_mbi.RegionSize / 2);            // so the buffer don"t overflow

                ZwReadVirtualMemory(GetCurrentProcess(), currentmemorypage, &buffer.at(0), m_mbi.RegionSize, 0);

                for (int begin = 0; begin < m_mbi.RegionSize; begin++)
                {
                    for (const auto& Item : m_Signatures)
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
                                    HMODULE hModule = reinterpret_cast<HMODULE>(m_mbi.AllocationBase);
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

        currentmemorypage += m_mbi.RegionSize;
    }
    SharedUtil::AddDebugLog("End Scan");
    currentmemorypage = 0;
}