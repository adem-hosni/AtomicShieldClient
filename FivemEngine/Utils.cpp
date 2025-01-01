#include "StdInc.h"
#include "Utils.h"

std::string Utils::ParseModuleNameFromPath(std::string strPath)
{
    if (strPath.empty())
        return strPath;
    return strPath.substr(strPath.find_last_of("/\\") + 1);
}

std::wstring Utils::ParseModuleNameFromPath(std::wstring wstrPath)
{
    if (wstrPath.empty())
        return wstrPath;
    return wstrPath.substr(wstrPath.find_last_of(L"/\\") + 1);
}

long Utils::GetFileSize(FILE* File)
{
    if (File == nullptr)
        return 0x0;

    long lCurrentPos, lEndPos;
    lCurrentPos = ftell(File);
    fseek(File, 0, 2);
    lEndPos = ftell(File);
    fseek(File, lCurrentPos, 0);
    return lEndPos;
}

DWORD Utils::GenerateCRC32(const std::string& filePath, DWORD* FileSize)
{
    if (filePath.empty())
        return 0x0;
    FILE* hFile = fopen(filePath.c_str(), "rb");
    if (hFile == nullptr)
        return 0x0;
    BYTE* fileBuf = nullptr;
    DWORD fileSize = GetFileSize(hFile);
    if (FileSize != nullptr)
        *FileSize = fileSize;
    fileBuf = new BYTE[fileSize];
    fread(fileBuf, fileSize, 1, hFile);
    fclose(hFile);
    DWORD crc = CRC::Calculate(fileBuf, fileSize, CRC::CRC_32());
    delete[] fileBuf;
    return crc;
}

DWORD Utils::GenerateCRC32(const std::wstring& wfilePath, DWORD* FileSize)
{
    if (wfilePath.empty())
        return 0x0;
    FILE* hFile = _wfopen(wfilePath.c_str(), L"rb");
    if (hFile == nullptr)
        return 0x0;
    BYTE* fileBuf = nullptr;
    DWORD fileSize = GetFileSize(hFile);
    if (FileSize != nullptr)
        *FileSize = fileSize;
    fileBuf = new BYTE[fileSize];
    fread(fileBuf, fileSize, 1, hFile);
    fclose(hFile);
    DWORD crc = CRC::Calculate(fileBuf, fileSize, CRC::CRC_32());
    delete[] fileBuf;
    return crc;
}

std::map<LPVOID, DWORD64> Utils::BuildModuledMemoryMap()
{
    std::map<LPVOID, DWORD64> memoryMap;
    HMODULE                   hMods[1024];
    DWORD                     cbNeeded;
    EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded);
    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
    {
        MODULEINFO modinfo;
        GetModuleInformation(GetCurrentProcess(), hMods[i], &modinfo, sizeof(modinfo));
        char buffer[144];
        memset(buffer, 0, sizeof(buffer));
        GetModuleFileName(hMods[i], buffer, sizeof(buffer));
        std::string modname = Utils::ParseModuleNameFromPath(buffer);
        
        memoryMap.insert(memoryMap.begin(), std::pair<LPVOID, DWORD64>(modinfo.lpBaseOfDll, modinfo.SizeOfImage));
    }
    return memoryMap;
}

int* Utils::GetModuleMemoryInfo(HANDLE hProcess, HMODULE Addr)
{
    if (Addr == nullptr)
        return nullptr;
    static MODULEINFO modinfo = {0};
    ZeroMemory(&modinfo, sizeof(MODULEINFO));
    __try
    {
        if (GetModuleInformation(hProcess, Addr, &modinfo, sizeof(MODULEINFO)))
            return (int*)&modinfo;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef ARTEMIS_DEBUG
        Utils::AddDebugLog("[SEH] 0x%X from GetModuleMemoryInfo!\n", GetExceptionCode());
#endif
    }
    return nullptr;
}

DWORD64 Utils::GetModuleBaseAddress(int iProcessID, std::string strModuleName)
{
    DWORD  dwBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, iProcessID);

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(hSnapshot, &moduleEntry))
        {
            do
            {
                // Compare the module name (moduleEntry.szModule) with the input string (moduleName)
                if (strModuleName == moduleEntry.szModule)
                {
                    dwBaseAddress = reinterpret_cast<DWORD64>(moduleEntry.modBaseAddr);
                    break;
                }
            } while (Module32Next(hSnapshot, &moduleEntry));
        }
    }
    CloseHandle(hSnapshot);
    return dwBaseAddress;
}

DWORD64 Utils::IsAddressInModuledRange(DWORD64 dwBase)
{
    std::map<LPVOID, DWORD64> memory = BuildModuledMemoryMap();
    for (const auto& it : memory)
    {
        if (dwBase >= (DWORD64)it.first && dwBase <= ((DWORD64)it.first + it.second))
            return (DWORD64)it.first;
    }
    return -1;
}

bool Utils::IsFunctionHooked(const char* szModuleName, const char* szFunctionName)
{
    if (szModuleName == nullptr || szFunctionName == nullptr)
        return false;

    HMODULE hModule = GetModuleHandle(szModuleName);
    if (hModule == NULL)
    {
        SharedUtil::AddDebugLog("Couldn't fetch module name '%s'!", szModuleName);
        return false;
    }

    DWORD64 pFunction = (DWORD64)GetProcAddress(hModule, szFunctionName);
    if (pFunction == NULL)
    {
        SharedUtil::AddDebugLog("Couldn't fetch address of function %s");
        return false;
    }

    // 0xEB = short jump, 0xE8 = call X, 0xE9 = long jump, 0xEA = "jmp oper2:oper1"
    __try
    {
        if (*(BYTE*)pFunction == 0xE8 || *(BYTE*)pFunction == 0xE9 || *(BYTE*)pFunction == 0xEA || *(BYTE*)pFunction == 0xEB)
            return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SharedUtil::AddDebugLog("Couldn't read bytes %s", szFunctionName);
    }

    return false;
}

std::string Utils::CaesarDecrypt(const std::string& ciphertext, int shift)
{
    std::string decrypted_text = "";
    for (char c : ciphertext)
    {
        if (isalpha(c))
        {                                                                         // Check if the character is a letter
            char base = isupper(c) ? 'A' : 'a';                                   // Determine base for uppercase or lowercase
            char decrypted_char = (c - base - shift + 26) % 26 + base;            // Subtract the shift
            decrypted_text += decrypted_char;
        }
        else
        {
            decrypted_text += c;            // Non-alphabetic characters remain unchanged
        }
    }
    return decrypted_text;
}

int Utils::RunPortableExecutable(void* Image, const std::vector<std::string>& args, char* outputBuffer, size_t outputBufferSize)
{
    IMAGE_DOS_HEADER*     DOSHeader;
    IMAGE_NT_HEADERS*     NtHeader;
    IMAGE_SECTION_HEADER* SectionHeader;

    PROCESS_INFORMATION PI;
    STARTUPINFOA        SI;

    CONTEXT* CTX;

    DWORD64 ImageBase;
    void*   pImageBase;

    int  count;
    char CurrentFilePath[1024];

    DOSHeader = PIMAGE_DOS_HEADER(Image);
    NtHeader = PIMAGE_NT_HEADERS(DWORD_PTR(Image) + DOSHeader->e_lfanew);

    GetModuleFileNameA(0, CurrentFilePath, 1024);

    if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
    {
        ZeroMemory(&PI, sizeof(PI));
        ZeroMemory(&SI, sizeof(SI));
        SI.cb = sizeof(SI);
        SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
        HANDLE              stdoutRead, stdoutWrite;

        if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0))
        {
            return 1;
        }

        SI.hStdOutput = stdoutWrite;
        SI.hStdError = stdoutWrite;
        SI.dwFlags |= STARTF_USESTDHANDLES;

        std::stringstream argStringStream;
        for (const auto& arg : args)
        {
            argStringStream << arg << " ";
        }
        std::string argString = argStringStream.str();

        SharedUtil::AddDebugLog(CurrentFilePath);

        if (CreateProcessA(CurrentFilePath, const_cast<char*>(argString.c_str()), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &SI, &PI))
        {
            CTX = LPCONTEXT(VirtualAlloc(NULL, sizeof(CONTEXT), MEM_COMMIT, PAGE_READWRITE));
            CTX->ContextFlags = CONTEXT_FULL;

            if (GetThreadContext(PI.hThread, LPCONTEXT(CTX)))
            {
                ReadProcessMemory(PI.hProcess, LPCVOID(CTX->Rdx + 0x10), LPVOID(&ImageBase), sizeof(DWORD64), 0);

                pImageBase = VirtualAllocEx(PI.hProcess, LPVOID(NtHeader->OptionalHeader.ImageBase), NtHeader->OptionalHeader.SizeOfImage,
                                            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

                WriteProcessMemory(PI.hProcess, pImageBase, Image, NtHeader->OptionalHeader.SizeOfHeaders, NULL);

                for (count = 0; count < NtHeader->FileHeader.NumberOfSections; count++)
                {
                    SectionHeader = IMAGE_FIRST_SECTION(NtHeader) + count;

                    WriteProcessMemory(PI.hProcess, LPVOID(DWORD_PTR(pImageBase) + SectionHeader->VirtualAddress),
                                       LPVOID(DWORD_PTR(Image) + SectionHeader->PointerToRawData), SectionHeader->SizeOfRawData, NULL);
                }

                WriteProcessMemory(PI.hProcess, LPVOID(CTX->Rdx + 0x10), LPVOID(&NtHeader->OptionalHeader.ImageBase), sizeof(DWORD64), NULL);

                CTX->Rcx = DWORD_PTR(pImageBase) + NtHeader->OptionalHeader.AddressOfEntryPoint;

                SetThreadContext(PI.hThread, LPCONTEXT(CTX));
                ResumeThread(PI.hThread);

                DWORD bytesRead;
                if (ReadFile(stdoutRead, outputBuffer, outputBufferSize - 1, &bytesRead, NULL))
                {
                    outputBuffer[bytesRead] = '\0';
                }

                CloseHandle(stdoutRead);
                CloseHandle(stdoutWrite);

                return 0;
            }
        }
    }

    return 1;
}