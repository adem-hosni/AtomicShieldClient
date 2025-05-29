#include <string>
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <random>
#include "SharedUtil.h"
#include <TlHelp32.h>

bool SharedUtil::TerminateProcess(DWORD dwPID)
{
    DWORD  dwDesiredAccess = PROCESS_TERMINATE;
    bool   bInheritHandle = FALSE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwPID);
    if (hProcess == NULL)
        return FALSE;

    bool result = ::TerminateProcess(hProcess, 0);

    CloseHandle(hProcess);

    return result;
}

bool SharedUtil::FindStringIC(const std::string& strHaystack, const std::string& strNeedle)
{
    auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(),
                          [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
    return (it != strHaystack.end());
}

int SharedUtil::GetProcessID(const char* szProcessName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;            // Error handling
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return 0;            // Error handling
    }

    do
    {
        if (strcmp(pe32.szExeFile, szProcessName) == 0)
        {
            CloseHandle(hSnapshot);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return NULL;            // Process not found
}

int SharedUtil::GetFivemProcessID()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;            // Error handling
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return 0;            // Error handling
    }

    auto IsFivemProcess = [](const std::string& strProcessName) -> bool
    {
        // List of base names
        static std::vector<std::string> baseNames = {"FiveM", "NVN"};

        static std::vector<std::string> suffixes = {"_GTAProcess.exe", "_GameProcess.exe"};

        for (const auto& base : baseNames)
        {
            if (strProcessName.starts_with(base))
            {
                for (const auto& suffix : suffixes)
                {
                    if (strProcessName.ends_with(suffix))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    do
    {
        if (IsFivemProcess(pe32.szExeFile))
        {
            CloseHandle(hSnapshot);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return NULL;            // Process not found
}

int SharedUtil::GenerateRandomNumber(int min, int max)
{
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

std::string SharedUtil::GenerateRandomString(int iLength)
{
    const static std::string              characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device                    rd;                         // Seed for the random number generator
    std::mt19937                          generator(rd());            // Mersenne Twister random number engine
    std::uniform_int_distribution<size_t> distribution(0, characters.size() - 1);

    std::string randomString;
    for (size_t i = 0; i < iLength; ++i)
    {
        randomString += characters[distribution(generator)];
    }

    return randomString;
}

bool SharedUtil::IsRunningAsAdministator()
{
    BOOL                     bIsAdmin = FALSE;
    PSID                     adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &bIsAdmin);
        FreeSid(adminGroup);
    }
    return bIsAdmin;
}

const char* SharedUtil::GetParentProcessName()
{
    ULONG_PTR pbi[6];
    ULONG     ulSize = 0;
    DWORD     dwPID = 0x0;
    LONG(WINAPI * NtQueryInformationProcess)
    (HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
    *(FARPROC*)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("ntdll.DLL"), "NtQueryInformationProcess");
    if (NtQueryInformationProcess)
    {
        if (NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
        {
            dwPID = pbi[5];
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
            if (!hProcess)
            {
                CloseHandle(hProcess);
                return "";
            }
            TCHAR szProcessName[MAX_PATH];
            if (!GetModuleFileNameEx(hProcess, 0, szProcessName, sizeof(szProcessName)))
                return "";
            return szProcessName;
        }
    }
    return "";
}

void SharedUtil::AddDebugLog(const char* szLog, ...)
{
    char localAppDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath)))
        return;

    char szLogDirectory[MAX_PATH];
    memset(szLogDirectory, 0, MAX_PATH);
    sprintf(szLogDirectory, "%s\\AtomicShield", localAppDataPath);

    CreateDirectory(szLogDirectory, NULL);

    char szNewFile[600];
    memset(szNewFile, 0, sizeof(szNewFile));
    sprintf(szNewFile, "%s\\Trace.logs", szLogDirectory);
    static bool bOnce = false;
    
    FILE* hFile = fopen(szNewFile, "a+");

    if (hFile)
    {
        time_t t = std::time(0);
        tm*    now = std::localtime(&t);
        char   szTimestamp[600];
        memset(szTimestamp, 0, sizeof(szTimestamp));
        sprintf(szTimestamp, "[%d:%d:%d] %s\n", now->tm_hour, now->tm_min, now->tm_sec, szLog);
        va_list args;
        va_start(args, szLog);
        vprintf(szTimestamp, args);
        vfprintf(hFile, szTimestamp, args);
        va_end(args);
        fclose(hFile);
    }
    else
    {
        printf("Failed to open file %s\n", szNewFile);
    }
}

bool SharedUtil::GetDebugLogs(std::string& szLog)
{
    char localAppDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath)))
        return false;
    char szLogDirectory[MAX_PATH];
    memset(szLogDirectory, 0, MAX_PATH);
    sprintf(szLogDirectory, "%s\\AtomicShield", localAppDataPath);
    char szNewFile[600];
    memset(szNewFile, 0, sizeof(szNewFile));
    sprintf(szNewFile, "%s\\Trace.logs", szLogDirectory);
    FILE* hFile = fopen(szNewFile, "rb");
    if (hFile)
    {
        fseek(hFile, 0, SEEK_END);
        long fileSize = ftell(hFile);
        fseek(hFile, 0, SEEK_SET);
        szLog.resize(fileSize);
        fread(&szLog[0], 1, fileSize, hFile);
        fclose(hFile);
        return true;
    }
    return false;
}

std::string SharedUtil::GetKnownDirectory(const KNOWNFOLDERID fid)
{
    PWSTR path = nullptr;
    char  szProgramDataDir[MAX_PATH];
    memset(szProgramDataDir, 0, sizeof(szProgramDataDir));

    HRESULT result = SHGetKnownFolderPath(fid, 0, NULL, &path);
    if (!FAILED(result))
    {
        wcstombs(szProgramDataDir, path, MAX_PATH);
        CoTaskMemFree(path);
    }
    return std::string(szProgramDataDir);
}

bool SharedUtil::SetPrivilege(LPCTSTR lpszPrivilege)
{
    HANDLE           hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &tkp.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

std::string SharedUtil::Base64Encode(std::string data)
{
    static const char base64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string encoded;
    int         val = 0, valb = -6;
    for (unsigned char c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4)
    {
        encoded.push_back('=');
    }
    return encoded;
}

std::wstring SharedUtil::Base64Encode(std::wstring data)
{
    static const wchar_t base64Chars[] =
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        L"abcdefghijklmnopqrstuvwxyz"
        L"0123456789+/";

    std::wstring encoded;
    int          val = 0, valb = -6;
    for (wchar_t c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4)
    {
        encoded.push_back(L'=');
    }
    return encoded;
}

std::string SharedUtil::Base64Decode(std::string& encoded_string)
{
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    auto is_base64 = [](unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); };

    size_t        in_len = encoded_string.size();
    size_t        i = 0;
    size_t        j = 0;
    int           in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string   decoded_string;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
            {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
            {
                decoded_string += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
        {
            char_array_4[j] = 0;
        }

        for (j = 0; j < 4; j++)
        {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < (i - 1); j++)
        {
            decoded_string += char_array_3[j];
        }
    }

    return decoded_string;
}

void SharedUtil::SetRegistryIntValue(const char* szKey, int iValue)
{
    HKEY hKey;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\AtomicShield", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, szKey, 0, REG_DWORD, (const BYTE*)&iValue, sizeof(iValue));
        RegCloseKey(hKey);
    }
    else
    {
        AddDebugLog("Failed to create or open registry key.");
    }
}