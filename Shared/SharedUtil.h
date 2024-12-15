#pragma once
#include <Windows.h>
#include <shlobj.h>

namespace SharedUtil
{
    bool        TerminateProcess(DWORD dwPID);
    int         GetProcessID(const char* szProcessName);
    int         GetFivemProcessID();
    bool        FindStringIC(const std::string& strHaystack, const std::string& strNeedle);
    int         GenerateRandomNumber(int min, int max);
    std::string GenerateRandomString(int iLength);
    const char* GetParentProcessName();
    bool        IsRunningAsAdministator();
    void        AddDebugLog(const char* szLog, ...);
    std::string GetKnownDirectory(const KNOWNFOLDERID fid);
}            // namespace SharedUtil