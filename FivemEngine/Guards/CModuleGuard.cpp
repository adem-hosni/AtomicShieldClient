#include "StdInc.h"

CModuleGuard::CModuleGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
    m_dwMaxAddress = NULL;
}

CModuleGuard::~CModuleGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

PLDR_DATA_TABLE_ENTRY GetNextNode(PCHAR node, int iOffset)
{
    node -= sizeof(LIST_ENTRY) * iOffset;
    return (PLDR_DATA_TABLE_ENTRY)node;
}

void CModuleGuard::DoPulse()
{
    std::wstring wstrFullDllName;
    while (true)
    {
        PROCESS_BASIC_INFORMATION PBI = {0};
        if (NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &PBI, sizeof(PROCESS_BASIC_INFORMATION), NULL)))
        {
            LDR_DATA_TABLE_ENTRY LdrModule;
            PPEB_LDR_DATA        pLdrData = nullptr;
            
            PBYTE LdrDataOffset = (PBYTE)PBI.PebBaseAddress + offsetof(struct _PEB, Ldr);
            if (LdrDataOffset != NULL)
            {
                pLdrData = *(PPEB_LDR_DATA*)LdrDataOffset;
                if (pLdrData)
                {
                    PBYTE EntryAddress = (PBYTE)pLdrData->InMemoryOrderModuleList.Flink;
                    EntryAddress -= sizeof(LIST_ENTRY) * 1;

                    PLDR_DATA_TABLE_ENTRY Head = (PLDR_DATA_TABLE_ENTRY)EntryAddress;
                    PLDR_DATA_TABLE_ENTRY Node = Head;

                    do
                    {
                        LdrModule = *(LDR_DATA_TABLE_ENTRY*)Node;
                        wstrFullDllName = std::wstring(LdrModule.FullDllName.Length / sizeof(WCHAR), 0);
                        wstrFullDllName = LdrModule.FullDllName.Buffer;
                        wstrFullDllName.push_back('\0');

                        std::wcout << "Module Found: " << wstrFullDllName.c_str() << std::endl;


                        Node = GetNextNode((PCHAR)LdrModule.InMemoryOrderLinks.Flink, 1);
                    } while (Head != Node);
                }
            }
            else
            {
                SharedUtil::AddDebugLog("Couldn't fetch Ldr Data offset");
            }
        }
        else
        {
            SharedUtil::AddDebugLog("NtQueryInformationProcess failed with error 0x%x", GetLastError());
        }
        Sleep(3000);
    }
}