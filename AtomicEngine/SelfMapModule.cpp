#include "StdInc.h"
#include <thread>
#include "XorStr.h"

#pragma runtime_checks("", off)
#pragma optimize("", off)

int SelfMapModule::MapModule(BYTE* pSrcData, SIZE_T FileSize, bool ClearHeader, bool ClearNonNeededSections, bool AdjustProtections, bool SEHExceptionSupport,
                               DWORD fdwReason, LPVOID lpReserved)
{
    IMAGE_NT_HEADERS*      pOldNtHeader = nullptr;
    IMAGE_OPTIONAL_HEADER* pOldOptHeader = nullptr;
    IMAGE_FILE_HEADER*     pOldFileHeader = nullptr;
    BYTE*                  pTargetBase = nullptr;

    if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D)
    {
        ILog("Invalid file");
        return 1;
    }

    pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
    pOldOptHeader = &pOldNtHeader->OptionalHeader;
    pOldFileHeader = &pOldNtHeader->FileHeader;

    if (pOldFileHeader->Machine != CURRENT_ARCH)
    {
        ILog("Invalid platform");
        return 2;
    }

    // Allocate memory in current process
    pTargetBase = reinterpret_cast<BYTE*>(VirtualAlloc(nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!pTargetBase)
    {
        ILog("Memory allocation failed 0x%X", GetLastError());
        return 3;
    }

    DWORD oldp = 0;
    VirtualProtect(pTargetBase, pOldOptHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &oldp);

    MANUAL_MAPPING_DATA data{0};
    data.pLoadLibraryA = LoadLibraryA;
    data.pGetProcAddress = GetProcAddress;
#ifdef _WIN64
    data.pRtlAddFunctionTable = (f_RtlAddFunctionTable)RtlAddFunctionTable;
#else
    SEHExceptionSupport = false;
#endif
    data.pbase = pTargetBase;
    data.fdwReasonParam = fdwReason;
    data.reservedParam = lpReserved;
    data.SEHSupport = SEHExceptionSupport;

    // Copy file header directly
    memcpy(pTargetBase, pSrcData, 0x1000);

    IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
    for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
    {
        if (pSectionHeader->SizeOfRawData)
        {
            memcpy(pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData);
        }
    }

    ILog("Mapped DLL at %p", pTargetBase);
    ILog("Data allocated");

#ifdef _DEBUG
    ILog("My shellcode pointer %p", Shellcode);
    system("pause");
#endif

    // Call shellcode directly in current process
    Shellcode(&data);

    HINSTANCE hCheck = data.hMod;

    if (hCheck == (HINSTANCE)0x404040)
    {
        ILog("Wrong mapping ptr");
        VirtualFree(pTargetBase, 0, MEM_RELEASE);
        return 12;
    }
    else if (hCheck == (HINSTANCE)0x505050)
    {
        ILog("WARNING: Exception support failed!");
    }

    BYTE* emptyBuffer = (BYTE*)malloc(4096);
    if (emptyBuffer == nullptr)
    {
        ILog("Unable to allocate memory");
        return 13;
    }
    memset(emptyBuffer, 0, 4096);

    // CLEAR PE HEAD
    if (ClearHeader)
    {
        memcpy(pTargetBase, emptyBuffer, 0x1000);
    }
    // END CLEAR PE HEAD

    if (ClearNonNeededSections)
    {
        pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
        for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
        {
            if (pSectionHeader->Misc.VirtualSize)
            {
                if ((SEHExceptionSupport ? 0 : strcmp((char*)pSectionHeader->Name, XorStr(".pdata").c_str()) == 0) ||
                    strcmp((char*)pSectionHeader->Name, XorStr(".rsrc").c_str()) == 0 || strcmp((char*)pSectionHeader->Name, XorStr(".reloc").c_str()) == 0)
                {
                    ILog("Processing %s removal", pSectionHeader->Name);
                    memset(pTargetBase + pSectionHeader->VirtualAddress, 0, pSectionHeader->Misc.VirtualSize);
                }
            }
        }
    }

    if (AdjustProtections)
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
                if (VirtualProtect(pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, newP, &old))
                {
                    ILog("section %s set as %lX", (char*)pSectionHeader->Name, newP);
                }
                else
                {
                    ILog("FAIL: section %s not set as %lX", (char*)pSectionHeader->Name, newP);
                }
            }
        }
        DWORD old = 0;
        VirtualProtect(pTargetBase, IMAGE_FIRST_SECTION(pOldNtHeader)->VirtualAddress, PAGE_READONLY, &old);
    }

    free(emptyBuffer);

    ILog("Injection completed successfully");
    return 0;
}

// The Shellcode function remains the same as in your original code
void __stdcall SelfMapModule::Shellcode(MANUAL_MAPPING_DATA* pData)
{
    if (!pData)
    {
        pData->hMod = (HINSTANCE)0x404040;
        return;
    }

    BYTE* pBase = pData->pbase;
    auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)pBase)->e_lfanew)->OptionalHeader;

    auto _LoadLibraryA = pData->pLoadLibraryA;
    auto _GetProcAddress = pData->pGetProcAddress;

#ifdef _WIN64
    auto _RtlAddFunctionTable = pData->pRtlAddFunctionTable;
#endif
    auto _DllMain = reinterpret_cast<f_DLL_ENTRY_POINT>(pBase + pOpt->AddressOfEntryPoint);

    BYTE* LocationDelta = pBase - pOpt->ImageBase;
    if (LocationDelta)
    {
        if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
        {
            auto*       pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
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
                        UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
                        *pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
                    }
                }
                pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
            }
        }
    }

    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
    {
        auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        while (pImportDescr->Name)
        {
            char*     szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
            HINSTANCE hDll = _LoadLibraryA(szMod);

            ULONG_PTR* pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
            ULONG_PTR* pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);

            if (!pThunkRef)
                pThunkRef = pFuncRef;

            for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
            {
                if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
                {
                    *pFuncRef = (ULONG_PTR)_GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
                }
                else
                {
                    auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
                    *pFuncRef = (ULONG_PTR)_GetProcAddress(hDll, pImport->Name);
                }
            }
            ++pImportDescr;
        }
    }

    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
    {
        auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
        auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
        for (; pCallback && *pCallback; ++pCallback)
            (*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
    }

    bool ExceptionSupportFailed = false;

#ifdef _WIN64
    if (pData->SEHSupport)
    {
        auto excep = pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        if (excep.Size)
        {
            if (!_RtlAddFunctionTable(reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY*>(pBase + excep.VirtualAddress),
                                      excep.Size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY), (DWORD64)pBase))
            {
                ExceptionSupportFailed = true;
            }
        }
    }
#endif

    _DllMain(pBase, pData->fdwReasonParam, pData->reservedParam);

    if (ExceptionSupportFailed)
        pData->hMod = reinterpret_cast<HINSTANCE>(0x505050);
    else
        pData->hMod = reinterpret_cast<HINSTANCE>(pBase);
}