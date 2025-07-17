#pragma once
#include <Windows.h>
#include <shlobj.h>
#include <string>

namespace SharedUtil
{
    bool         TerminateProcess(DWORD dwPID);
    int          GetProcessID(const char* szProcessName);
    int          GetFivemProcessID();
    bool         FindStringIC(const std::string& strHaystack, const std::string& strNeedle);
    int          GenerateRandomNumber(int min, int max);
    std::string  GenerateRandomString(int iLength);
    const char*  GetParentProcessName();
    bool         IsRunningAsAdministator();
    void         AddDebugLog(const char* szLog, ...);
    bool         GetDebugLogs(std::string& szLog);
    std::string  GetKnownDirectory(const KNOWNFOLDERID fid);
    bool         SetPrivilege(LPCTSTR lpszPrivilege);
    std::string  Base64Encode(std::string data);
    std::wstring Base64Encode(std::wstring data);
    std::string  Base64Decode(std::string& data);
    void         SetRegistryIntValue(const char* ss,const char* szKey, int iValue);
}            // namespace SharedUtil