#include "StdInc.h"

#define START_ADDRESS (PVOID)0x00000000010000
#define END_ADDRESS   (0x00007FF8F2580000 - 0x00000000010000)

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
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
}

void CHeuristicGuard::DoPulse()
{
    char* text = (char*)"api.tzproject.com";
    const char* text2 = "api.tzproject.com";
    while (true)
    {
        SharedUtil::AddDebugLog("Begin scan");
        char* currentmemorypage = 0;

        MEMORY_BASIC_INFORMATION info;
        DWORD                    mask = (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ);

        const void* pCurrentAddress = START_ADDRESS;
        const void* end = (const void*)((const char*)pCurrentAddress + END_ADDRESS);
        while (pCurrentAddress < end && VirtualQuery(pCurrentAddress, &info, sizeof(info)) == sizeof(info))
        {
            if ((info.State != MEM_FREE && info.State != MEM_RELEASE) && info.Type & (MEM_IMAGE | MEM_PRIVATE) && info.Protect & mask)
            {
                std::string buffer;
                buffer.resize(info.RegionSize + info.RegionSize / 2);            // so the buffer don"t overflow

                ZwReadVirtualMemory(GetCurrentProcess(), currentmemorypage, &buffer.at(0), info.RegionSize, 0);

                for (int begin = 0; begin < info.RegionSize; begin++)
                {
                    for (const auto& Item : m_Signatures)
                    {
                        std::string              SignatureTitle = Item.first;
                        std::vector<std::string> SignaturesList = Item.second;

                        for (std::string& Signature : SignaturesList)
                        {
                            if (buffer[begin] == Signature.at(0) /*&& buffer[begin + Signature.length() - 1] == Signature.back()*/)
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
            pCurrentAddress = (const void*)((const char*)(info.BaseAddress) + info.RegionSize);
        }
        SharedUtil::AddDebugLog("End scan");

    }
}