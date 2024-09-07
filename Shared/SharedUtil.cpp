#include <string>
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

int SharedUtil::GenerateRandomNumber(int min, int max)
{
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}