#include "StdInc.h"

CModuleGuard::CModuleGuard()
{
}

CModuleGuard::~CModuleGuard()
{
}

PLDR_DATA_TABLE_ENTRY GetNextNode(PCHAR node, int iOffset)
{
    node -= sizeof(LIST_ENTRY) * iOffset;
    return (PLDR_DATA_TABLE_ENTRY)node;
}




NTSTATUS __stdcall CModuleGuard::_LdrLoadDll(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress)
{
    INT   i;
    DWORD dwOldProtect;
    BOOL  bAllow = FALSE;
    DWORD dwbytesWritten;
    CHAR  cDllName[MAX_PATH];
    sprintf(cDllName, "%S", DllName->Buffer);
    for (i = 0; i < dwAllowDllCount; i++)
    {
        if (strcmp(cDllName, cAllowDlls[i]) == 0)
        {
            bAllow = TRUE;

            printf("Allowing DLL: %s\n", cDllName);

            VirtualProtect(lpAddr, sizeof(OriginalBytes), PAGE_EXECUTE_READWRITE, &dwOldProtect);
            memcpy(lpAddr, OriginalBytes, sizeof(OriginalBytes));
            VirtualProtect(lpAddr, sizeof(OriginalBytes), dwOldProtect, &dwOldProtect);

            LdrLoadDll_ LdrLoadDll = (LdrLoadDll_)GetProcAddress(LoadLibrary("ntdll.dll"), "LdrLoadDll");

            LdrLoadDll(SearchPath, DllCharacteristics, DllName, BaseAddress);

            HookLoadDll(lpAddr);
        }
    }

    if (!bAllow)
    {
        printf("Blocked DLL: %s\n", cDllName);
    }
    SMemoryDetectionReport report{0};
    g_pSafeAntiCheat->NotifyDetection(eDetectionType::INJECTED_DLL, &report);
    return 0;
}

VOID CModuleGuard::HookLoadDll(LPVOID lpAddr)
{
    DWORD oldProtect, oldOldProtect;
    void* hLdrLoadDll = &CModuleGuard::_LdrLoadDll;

    // our trampoline
    unsigned char boing[] = { 0x49, 0xbb, 0xde, 0xad, 0xc0, 0xde, 0xde, 0xad, 0xc0, 0xde, 0x41, 0xff, 0xe3 };

    // add in the address of our hook
    *(void**)(boing + 2) = &CModuleGuard::_LdrLoadDll;

    // write the hook
    VirtualProtect(lpAddr, 13, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(lpAddr, boing, sizeof(boing));
    VirtualProtect(lpAddr, 13, oldProtect, &oldProtect);

    return;
}


void CModuleGuard::DoPulse()
{
    LPVOID lpAddr = (LPVOID)GetProcAddress(GetModuleHandle("ntdll.dll"), "LdrLoadDll");
    BYTE OriginalBytes[50] = {0};
    memcpy(OriginalBytes, lpAddr, 50);

    HookLoadDll(lpAddr);

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

                        if (!wstrFullDllName.empty())
                        {
                            if (!FileAuthentication::HasSignature(wstrFullDllName.c_str()))
                            {
                                SharedUtil::AddDebugLog("Unsigned module detected");
                                wprintf(L"File Path: %s\n", wstrFullDllName.c_str());
                            }
                        }

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
