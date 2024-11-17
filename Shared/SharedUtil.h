#pragma once
#include <Windows.h>

namespace SharedUtil
{
    bool TerminateProcess(DWORD dwPID);
    int  GetProcessID(const char* szProcessName);
    bool FindStringIC(const std::string& strHaystack, const std::string& strNeedle);
    int  GenerateRandomNumber(int min, int max);
    const char* GetParentProcessName();
    bool IsRunningAsAdministator();
}            // namespace SharedUtil