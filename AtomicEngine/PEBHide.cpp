#include "PEBHide.h"

void PEBHide::EraseSelfPEHeader(LPVOID lpBaseAddress)
{
    DWORD dwOldProtect = NULL;

    MODULEINFO  modInfo;
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    VirtualProtect(lpBaseAddress, systemInfo.dwPageSize, PAGE_READWRITE, &dwOldProtect);
    RtlSecureZeroMemory(lpBaseAddress, systemInfo.dwPageSize);
}

void PEBHide::UnlinkSelfLdrModule(LPVOID lpBaseAddress)
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
                LIST_ENTRY* Head = &pLdrData->InMemoryOrderModuleList;
                LIST_ENTRY* pCurrentEntry = Head->Flink;
              
                while (pCurrentEntry != Head)
                {
                    PLDRDATA_TABLE_ENTRY Node = CONTAINING_RECORD(pCurrentEntry, LDRDATA_TABLE_ENTRY, InMemoryOrderLinks);

                    if (Node->DllBase == lpBaseAddress)
                    {
                        memset(Node->BaseDllName.Buffer, 0, Node->BaseDllName.Length);
                        memset(Node->FullDllName.Buffer, 0, Node->FullDllName.Length);

                        UNLINK(Node->InLoadOrderLinks);
                        UNLINK(Node->InInitializationOrderLinks);
                        UNLINK(Node->InMemoryOrderLinks);

                        /*Head->Blink->Flink = Head->Flink;
                        Head->Flink->Blink = Head->Blink;*/

                        Node->HashLinks.Blink->Flink = Node->HashLinks.Flink;
                        Node->HashLinks.Flink->Blink = Node->HashLinks.Blink;

                        return;
                    }

                    pCurrentEntry = pCurrentEntry->Flink;
                }
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
}
