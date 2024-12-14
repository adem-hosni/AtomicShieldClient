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

void Utils::BuildModuledMemoryMap(HANDLE hProcess)
{
    HMODULE hMods[1024]{nullptr};
    DWORD   cbNeeded = NULL;
    if (K32EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        DWORD MdlCount = (cbNeeded / sizeof(HMODULE));
        for (unsigned int i = 0; i < MdlCount; i++)
        {
            if (hMods[i] == nullptr)
                continue;
            LPMODULEINFO modinfo = (LPMODULEINFO)GetModuleMemoryInfo(hProcess, hMods[i]);
            if (modinfo != nullptr)
            {
                if (orderedMapping.count((DWORD)modinfo->lpBaseOfDll) != 0x1)
                {
                    orderedMapping.insert(std::pair<DWORD, DWORD>((DWORD)modinfo->lpBaseOfDll, modinfo->SizeOfImage));
                    WCHAR wszFileName[MAX_PATH + 1];
                    if (!GetModuleFileNameW((HMODULE)modinfo->lpBaseOfDll, wszFileName, MAX_PATH + 1))
                        return;
                    DWORD        CRC32 = GenerateCRC32(wszFileName, nullptr);
                    std::wstring DllName = ParseModuleNameFromPath(wszFileName);
                    if (orderedIdentify.count(CRC32) != 0x1)
                        orderedIdentify.insert(orderedIdentify.begin(), std::pair<DWORD, std::wstring>(CRC32, DllName));
                }
            }
        }
    }
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
    DWORD dwBaseAddress = 0;
    HANDLE    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, iProcessID);

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

bool Utils::IsAddressInModuledRange(const DWORD dwBase, const std::string& strMappedName, bool* bCloacking)
{
    if (!orderedMapping.size())
    {
        BuildModuledMemoryMap(g_pSafeAntiCheat->GetProcessHandle());
        SharedUtil::AddDebugLog("Moduled Memory Map Size: %d", orderedMapping.size());
    }

    if (dwBase == NULL)
        return false;

    for (const auto& it : orderedMapping)
    {
        if (dwBase >= it.first && dwBase <= (it.first + it.second))
            return true;
    }
    if (!strMappedName.empty() && strMappedName.length() > 4)
    {
        if (GetModuleBaseAddress(g_pSafeAntiCheat->GetProcessID(), strMappedName) == dwBase)
            return true;
        else
        {
            if (bCloacking != nullptr)
                *bCloacking = true;
        }
    }
    return false;
}