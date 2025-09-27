#include "StaticAnalysisBypass.h"

std::string StaticAnalysisBypass::executeCommand(const char* szCmd)
{
    std::string         command = szCmd;
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
    /*IP_ADAPTER_INFO* pAdapterInfo = nullptr;
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
    free(pAdapterInfo);*/
    return "Unknown MAC";
}

std::string StaticAnalysisBypass::GetHWID()
{
    std::string hwid = executeCommand(skCrypt("wmic csproduct get uuid"));
    std::regex  regex("\\s+");
    std::string cleaned_hwid = std::regex_replace(hwid, regex, " ");
    return cleaned_hwid.substr(cleaned_hwid.find_first_not_of(" "), cleaned_hwid.find_last_not_of(" ") + 1);
}

std::string StaticAnalysisBypass::GetGPUInfo()
{
    std::string        gpu_info = executeCommand(skCrypt("wmic path win32_videocontroller get caption"));
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
    std::string              system_info = executeCommand(skCrypt("systeminfo"));
    std::vector<std::string> vm_indicators = {"VBOX", "VIRTUALBOX", "VMWARE", "XEN", "QEMU", "VIRTUAL", "HYPERVISOR", "SBOX", "SANDBOX", "CWSANDBOX"};
    std::vector<std::string> analysis_indicators = {"virustotal", "hybrid-analysis", "cuckoo",    "malwr",   "any.run", "reverse.it", "joe sandbox", "threatgrid", "cape sandbox",    "totalhash", "intezer", "ahnlab",  "AhnLab"};

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
    // Detect common analysis VMs and blacklisted hardware and log a number indicates the detection

    std::string ip_address = GetIPAddress();
    std::string mac_address = GetMACAddress();
    std::string hwid = GetHWID();
    std::string gpu = GetGPUInfo();
    std::string username = GetUsername();

    if (DetectEnvironment())
    {
        SharedUtil::AddDebugLog("Environment detection triggered");
        return true;
    }

    for (const auto& bannedGpu : blacklisted_gpus)
    {
        if (gpu.find(bannedGpu) != std::string::npos)
        {
            SharedUtil::AddDebugLog("Detected 1");
            return true;
        }
    }

    for (const auto& bannedHwid : blacklisted_hwids)
    {
        if (hwid.find(bannedHwid) != std::string::npos)
        {
            SharedUtil::AddDebugLog("Detected 2");
            return true;
        }
    }

    for (const auto& bannedIp : blacklisted_ips)
    {
        if (ip_address.find(bannedIp) != std::string::npos)
        {
            SharedUtil::AddDebugLog("Detected 3");
            return true;
        }
    }

    for (const auto& bannedMac : blacklisted_macs)
    {
        if (mac_address.find(bannedMac) != std::string::npos)
        {
            SharedUtil::AddDebugLog("Detected 4");
            return true;
        }
    }

    for (const auto& bannedUser : blacklistUsers)
    {
        if (username.find(bannedUser) != std::string::npos)
        {
            SharedUtil::AddDebugLog("Detected 5");
            return true;
        }
    }
    return false;

    // return false;
}