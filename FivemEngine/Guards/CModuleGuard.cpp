#include "StdInc.h"
#include <synchapi.h>

#define dwAllowDllCount 1

CModuleGuard::CModuleGuard()
{
    cAllowDlls = {"C:\\Windows\\System32\\rsaenh.dll"};
    OriginalBytes[50] = {0};
    lpAddr = nullptr;
}

CModuleGuard::~CModuleGuard()
{
}

NTSTATUS NTAPI _LdrLoadDll(PWCHAR PathToFile_OPTIONAL, ULONG Flags, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle)
{
    SharedUtil::AddDebugLog("Hook call");
    INT   i;
    DWORD dwOldProtect;
    BOOL  bAllow = FALSE;
    DWORD dwbytesWritten;
    CHAR  cDllName[MAX_PATH];
    sprintf(cDllName, "%S", ModuleFileName->Buffer);
    // for (i = 0; i < dwAllowDllCount; i++)
    //{
    //     if (strcmp(cDllName, cAllowDlls[i]) == 0)
    //     {
    //         bAllow = TRUE;

    //        printf("Allowing DLL: %s\n", cDllName);

    //        VirtualProtect(lpAddr, sizeof(OriginalBytes), PAGE_EXECUTE_READWRITE, &dwOldProtect);
    //        memcpy(lpAddr, OriginalBytes, sizeof(OriginalBytes));
    //        VirtualProtect(lpAddr, sizeof(OriginalBytes), dwOldProtect, &dwOldProtect);

    //        LdrLoadDll_ LdrLoadDll = (LdrLoadDll_)GetProcAddress(LoadLibrary("ntdll.dll"), "LdrLoadDll");

    //        LdrLoadDll(SearchPath, DllCharacteristics, DllName, BaseAddress);

    //     //   HookLoadDll(lpAddr);
    //    }
    //}

    if (!bAllow)
    {
        printf("Blocked DLL: %s\n", cDllName);
    }
    SMemoryDetectionReport report{0};
    g_pSafeAntiCheat->NotifyDetection(eDetectionType::INJECTED_DLL, &report);
    return 0;
}

void CModuleGuard::Initialize()
{
    lpAddr = (LPVOID)GetProcAddress(LoadLibrary("ntdll.dll"), "LdrLoadDll");
    CAtomicHook::Create(lpAddr, &_LdrLoadDll);
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
