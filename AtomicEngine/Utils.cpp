#include "StdInc.h"
#include "Utils.h"
#include <psapi.h>
#include <tlhelp32.h>
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

bool Utils::isFiveMReady()
{
    static std::vector<std::string> chromeNames = {"FiveM_ChromeBrowser", "NVN_ChromeBrowser", "GvR_ChromeBrowser"};
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            for (const auto& name : chromeNames)
            {
                if (_stricmp(pe32.szExeFile, name.c_str()) == 0)
                {
                    CloseHandle(hSnapshot);
                    return true;
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return false;
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

std::vector<ModuleInfo> Utils::BuildModuledMemoryMap(HANDLE hProcess)
{
    std::vector<ModuleInfo> modules;
    HMODULE                 hMods[1024];
    DWORD                   cbNeeded;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            MODULEINFO modInfo;
            if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
            {
                char szModName[MAX_PATH];
                if (GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName)))
                {
                    modules.push_back({reinterpret_cast<DWORD64>(modInfo.lpBaseOfDll), modInfo.SizeOfImage, std::string(szModName)});
                }
            }
        }
    }
    return modules;
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

bool Utils::IsAddressInModuledRange(DWORD64 address, const std::vector<ModuleInfo>& memoryMap)
{
    for (const auto& module : memoryMap)
    {
        if (address >= module.BaseAddress && address < (module.BaseAddress + module.Size))
        {
            return true;
        }
    }
    return false;
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

std::wstring Utils::CaesarDecrypt(const std::wstring& ciphertext, int shift)
{
    std::wstring decrypted;
    shift = shift % 26;

    for (wchar_t ch : ciphertext)
    {
        if (ch >= L'A' && ch <= L'Z')
        {
            decrypted += (ch - L'A' - shift + 26) % 26 + L'A';
        }
        else if (ch >= L'a' && ch <= L'z')
        {
            decrypted += (ch - L'a' - shift + 26) % 26 + L'a';
        }
        else
        {
            decrypted += ch;
        }
    }

    return decrypted;
}


std::string Utils::GetFivemPath()
{
    return std::string(SharedUtil::GetKnownDirectory(FOLDERID_LocalAppData) + "\\FiveM\\FiveM.app");
}
