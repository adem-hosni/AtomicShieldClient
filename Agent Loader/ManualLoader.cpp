#include "StdInc.h"

bool ManualLoadBuffer(BYTE* pSrcData, DWORD dwSize)
{
    IMAGE_NT_HEADERS*      pOldNtHeader = nullptr;
    IMAGE_OPTIONAL_HEADER* pOldOptHeader = nullptr;
    IMAGE_FILE_HEADER*     pOldFileHeader = nullptr;
    BYTE*                  pTargetBase = nullptr;
    HANDLE                 hProc = GetCurrentProcess();

    BYTE* emptyBuffer = (BYTE*)malloc(4096);
    if (emptyBuffer == nullptr)
    {
        printf("Unable to allocate memory");
        return false;
    }
    memset(emptyBuffer, 0, sizeof(emptyBuffer));

    if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D)
    {            //"MZ"
        printf("Invalid file");
        return false;
    }

    pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
    pOldOptHeader = &pOldNtHeader->OptionalHeader;
    pOldFileHeader = &pOldNtHeader->FileHeader;

    if (pOldFileHeader->Machine != CURRENT_ARCH)
    {
        printf("Invalid platform\n");
        return false;
    }

    pTargetBase =
        reinterpret_cast<BYTE*>(RuntimeImportResolver::VirtualAllocEx(hProc, nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!pTargetBase)
    {
        printf("Target process memory allocation failed (ex) 0x%X\n", GetLastError());
        char szErrorMsg[64];
        printf(szErrorMsg);
        return false;
    }

    if (!RuntimeImportResolver::WriteProcessMemory(hProc, pTargetBase, pSrcData, 0x1000, nullptr))
    {
        printf("Can't write file header 0x%X\n", GetLastError());
        RuntimeImportResolver::VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
        return false;
    }

    IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
    for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
    {
        if (pSectionHeader->SizeOfRawData)
        {
            if (!RuntimeImportResolver::WriteProcessMemory(hProc, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData,
                                                           pSectionHeader->SizeOfRawData, nullptr))
            {
                printf("Can't map sections: 0x%x", GetLastError());
                RuntimeImportResolver::VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
                return false;
            }
        }
    }

    auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pTargetBase + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)pTargetBase)->e_lfanew)->OptionalHeader;
    auto  _DllMain = reinterpret_cast<f_DLL_ENTRY_POINT>(pTargetBase + pOpt->AddressOfEntryPoint);

    BYTE* LocationDelta = pTargetBase - pOpt->ImageBase;
    if (LocationDelta)
    {
        if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
        {
            auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pTargetBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
            const auto* pRelocEnd =
                reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
            while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock)
            {
                UINT  AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);

                for (UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo)
                {
                    if (RELOC_FLAG(*pRelativeInfo))
                    {
                        UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pTargetBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
                        *pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
                    }
                }
                pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
            }
        }
    }

    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
    {
        auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pTargetBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        while (pImportDescr->Name)
        {
            char* szMod = reinterpret_cast<char*>(pTargetBase + pImportDescr->Name);

            HINSTANCE hDll = LoadLibraryA(szMod);

            ULONG_PTR* pThunkRef = reinterpret_cast<ULONG_PTR*>(pTargetBase + pImportDescr->OriginalFirstThunk);
            ULONG_PTR* pFuncRef = reinterpret_cast<ULONG_PTR*>(pTargetBase + pImportDescr->FirstThunk);

            if (!pThunkRef)
                pThunkRef = pFuncRef;

            for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
            {
                if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
                {
                    *pFuncRef = (ULONG_PTR)GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
                }
                else
                {
                    auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pTargetBase + (*pThunkRef));
                    *pFuncRef = (ULONG_PTR)GetProcAddress(hDll, pImport->Name);
                }
            }
            ++pImportDescr;
        }
    }

    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
    {
        auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pTargetBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
        auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
        for (; pCallback && *pCallback; ++pCallback)
            (*pCallback)(pTargetBase, DLL_PROCESS_ATTACH, nullptr);
    }

    bool ExceptionSupportFailed = false;
    printf("DllMain at: 0x%p", _DllMain);


    if (true)
    {
        pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
        for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
        {
            if (pSectionHeader->Misc.VirtualSize)
            {
                if ((false ? 0 : strcmp((char*)pSectionHeader->Name, ".pdata") == 0) || strcmp((char*)pSectionHeader->Name, ".rsrc") == 0 ||
                    strcmp((char*)pSectionHeader->Name, ".reloc") == 0)
                {
                    printf("Processing %s removal", pSectionHeader->Name);
                    if (!RuntimeImportResolver::WriteProcessMemory(hProc, pTargetBase + pSectionHeader->VirtualAddress, emptyBuffer,
                                                                   pSectionHeader->Misc.VirtualSize, nullptr))
                    {
                        printf("Can't clear section %s: 0x%x", pSectionHeader->Name, GetLastError());
                    }
                }
            }
        }
    }

    {
        pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
        for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
        {
            if (pSectionHeader->Misc.VirtualSize)
            {
                DWORD old = 0;
                DWORD newP = PAGE_READONLY;

                if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) > 0)
                {
                    newP = PAGE_READWRITE;
                }
                else if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) > 0)
                {
                    newP = PAGE_EXECUTE_READ;
                }
                if (RuntimeImportResolver::VirtualProtectEx(hProc, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, newP, &old))
                {
                    printf("section %s set as %lX\n", (char*)pSectionHeader->Name, newP);
                }
                else
                {
                    printf("FAIL: section %s not set as %lX\n", (char*)pSectionHeader->Name, newP);
                }
            }
        }
        DWORD old = 0;
        RuntimeImportResolver::VirtualProtectEx(hProc, pTargetBase, IMAGE_FIRST_SECTION(pOldNtHeader)->VirtualAddress, PAGE_READONLY, &old);
    }

    bool bResult = _DllMain(pTargetBase, DLL_PROCESS_ATTACH, nullptr);

    return true;
}