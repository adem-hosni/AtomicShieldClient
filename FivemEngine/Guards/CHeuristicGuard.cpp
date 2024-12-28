#include "StdInc.h"
#pragma comment(lib, "ntdll.lib")

#define START_ADDRESS (PVOID)0x00000000010000
#define END_ADDRESS   (0x00007FF8F2580000 - 0x00000000010000)

typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation
} MEMORY_INFORMATION_CLASS;

extern "C" NTSYSCALLAPI NTSTATUS ZwReadVirtualMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);

extern "C" NTSYSCALLAPI NTSTATUS ZwWriteVirtualMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);

extern "C" NTSYSCALLAPI NTSTATUS NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass,
                                                      PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

void SearchForString(LPVOID lpAddress)
{
    std::string strSignature = Utils::CaesarDecrypt(*reinterpret_cast<std::string*>(lpAddress), 3);
    SYSTEM_INFO si;
    char*       currentmemorypage = 0;
    GetSystemInfo(&si);
    MEMORY_BASIC_INFORMATION info;
    bool                     bFirstScan = true;

    while (true)
    {
        while (currentmemorypage < si.lpMaximumApplicationAddress)
        {
            NtQueryVirtualMemory(GetCurrentProcess(), currentmemorypage, MemoryBasicInformation, &info, sizeof(info), 0);

            if (info.State == MEM_COMMIT)
            {
                if (info.Protect == PAGE_READWRITE)
                {
                    std::string buffer;
                    buffer.resize(info.RegionSize + info.RegionSize / 2);            // so the buffer don"t overflow

                    ZwReadVirtualMemory(GetCurrentProcess(), currentmemorypage, &buffer.at(0), info.RegionSize, 0);

                    for (int begin = 0; begin < info.RegionSize; begin++)
                    {
                        if (buffer[begin] == strSignature.at(0) && buffer[begin + strSignature.length() - 1] == strSignature.back())
                        {
                            std::string stringbuffer = buffer.substr(begin, strSignature.length());

                            if (strSignature.find(stringbuffer) != std::string::npos)
                            {
                                if (((DWORD64)currentmemorypage + begin) == (DWORD64)strSignature.data())
                                    continue;

                                if (bFirstScan)
                                {
                                    bFirstScan = false;
                                    continue;
                                }

                                char szModulePath[MAX_PATH];
                                memset(szModulePath, 0, sizeof(szModulePath));
                                HMODULE hModule = reinterpret_cast<HMODULE>(info.AllocationBase);
                                if (!GetModuleFileName(hModule, szModulePath, MAX_PATH))
                                    strcat(szModulePath, "<UNKNOWN>");
                                SharedUtil::AddDebugLog("Found \"%s\" at 0x%p in thread %d in %s", strSignature.c_str(), (DWORD64)currentmemorypage + begin,
                                                        GetCurrentThreadId(), szModulePath);
                                g_pSafeAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"signature", strSignature.c_str()},
                                                                                          {"found_at", (DWORD64)(currentmemorypage + begin)},
                                                                                          {"possible_module_path", szModulePath}});
                            }
                        }
                    }
                }
            }
            currentmemorypage += info.RegionSize;
        }

        currentmemorypage = 0;
        Sleep(1000);
    }
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            auto        sig = Signature;
            //CAtomicThread::Create(&SearchForString, reinterpret_cast<PVOID>(&sig));
            std::thread t(&SearchForString, reinterpret_cast<LPVOID>(&sig));
            
        }

        /*if (m_Signatures.count(name))
            m_Signatures[name].insert(m_Signatures[name].end(), std::make_move_iterator(vector.begin()), std::make_move_iterator(vector.end()));
        else
            m_Signatures[name] = std::move(vector);*/
    }
}
