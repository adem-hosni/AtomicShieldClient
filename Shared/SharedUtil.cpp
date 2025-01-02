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
        static std::string baseName = "FiveM_b";
        static std::string suffix = "_GTAProcess.exe";
        return strProcessName.size() > baseName.size() + suffix.size() && strProcessName.substr(0, baseName.size()) == baseName &&
               strProcessName.substr(strProcessName.size() - suffix.size()) == suffix;
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
    std::string log_name = "Trace.logs";
    char*       szLogDirectory = (char*)"C:\\AtomicShield\\AtomicShieldClient\\AtomicEngine";
    char        szNewDirectory[600];
    memset(szNewDirectory, 0, sizeof(szNewDirectory));
    sprintf(szNewDirectory, "%s\\%s", szLogDirectory, log_name.c_str());
    static bool bOnce = false;
    if (!bOnce)
    {
        FILE* hFile = fopen(szNewDirectory, "rb");
        if (hFile)
        {
            fclose(hFile);
            DeleteFileA(szNewDirectory);
        }
        bOnce = true;
    }
    FILE* hFile = fopen(szNewDirectory, "a+");
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
}

std::string SharedUtil::GetKnownDirectory(const KNOWNFOLDERID fid)
{
    PWSTR   path = nullptr;
    char szProgramDataDir[MAX_PATH];
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