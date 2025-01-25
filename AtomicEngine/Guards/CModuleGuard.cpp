#include "StdInc.h"
#include <synchapi.h>
#include <cwchar>

#define dwAllowDllCount 1

CModuleGuard::CModuleGuard()
{
    m_vAllowedModules = {
        L"icuuc.dll",
        L"icui18n.dll",
        L"chrome_elf.dll",
        L"libEGL.dll",
        L"libGLESv2.dll",
        L"libcef.dll",
        L"ros.dll",
        L"gfsdk_shadowlib.dll",
        L"SwiftShaderD3D9_64.dll",
        L"FiveM_b3095_GTAProcess.exe",
        L"FiveM_b2699_GTAProcess.exe"
    };
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
    g_pAtomicAntiCheat->NotifyDetection(eDetectionType::INJECTED_DLL, {{"dll_path", cDllName}});
    return 0;
}

void CModuleGuard::Initialize()
{
    lpAddr = (LPVOID)GetProcAddress(LoadLibrary("ntdll.dll"), "LdrLoadDll");
  //  CAtomicHook::Create(lpAddr, &_LdrLoadDll);
}

PLDR_DATA_TABLE_ENTRY GetNextNode(PCHAR node, int iOffset)
{
    node -= sizeof(LIST_ENTRY) * iOffset;
    return (PLDR_DATA_TABLE_ENTRY)node;
}

void CModuleGuard::DoPulse()
{
    std::wstring wstrModulePath;
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
                        wstrModulePath = std::wstring(LdrModule.FullDllName.Buffer, LdrModule.FullDllName.Length / sizeof(WCHAR));

                        if (!wstrModulePath.empty())
                        {
                            std::wstring wstrModuleName = Utils::ParseModuleNameFromPath(wstrModulePath);
                            bool         isWhitelisted = false;

                            // Check if the module is in the whitelist
                            for (const wchar_t* allowedModule : m_vAllowedModules)
                            {
                                if (_wcsicmp(wstrModuleName.c_str(), allowedModule) == 0)
                                {
                                    isWhitelisted = true;
                                    break;
                                }
                            }

                            // Skip logging or flagging if the module is whitelisted
                            if (isWhitelisted)
                            {
                                continue;
                            }

                            // Perform further checks for unsigned or suspicious modules
                            if (!FileAuthentication::HasSignature(wstrModulePath.c_str()))
                            {
                                static std::string strFivemPath = Utils::GetFivemPath();
                                if (!wstrModulePath._Starts_with(std::wstring(strFivemPath.begin(), strFivemPath.end())))
                                {
                                    g_pAtomicAntiCheat->NotifyDetection(INJECTED_DLL,
                                                                        {{"file_path", std::string(wstrModulePath.begin(), wstrModulePath.end())}});
                                }
                                else
                                {
                                    SharedUtil::AddDebugLog("Unsigned module in FiveM directory: %S", wstrModulePath.c_str());
                                }
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
    }
}
