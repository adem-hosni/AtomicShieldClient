#pragma once
#include <Windows.h>
#include <shlobj.h>
#include <string>

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
    bool        SetPrivilege(LPCTSTR lpszPrivilege);
    std::string Base64Encode(std::string& data);
    std::string Base64Decode(std::string& data);
}            // namespace SharedUtil