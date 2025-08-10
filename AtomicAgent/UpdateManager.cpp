#include "StdInc.h"

void UpdateManager::InstallAgent(std::string& AgentBuffer, std::string& strTitleStatus, std::string& strMessageStatus)
{
    if (AgentBuffer.empty())
    {
        SharedUtil::AddDebugLog(skCrypt("Failed to download agent, buffer is empty."));
        strTitleStatus = skCrypt("Agent Installation Failed");
        strMessageStatus = skCrypt("The AtomicShield Agent could not be downloaded. Please check your internet connection and try again.");
        return;
    }

    char szAgentPath[MAX_PATH];
    memset(szAgentPath, 0, sizeof(szAgentPath));
    DWORD dwSize = GetModuleFileNameA(NULL, szAgentPath, MAX_PATH);
    if (dwSize == 0 || dwSize == MAX_PATH)
    {
        SharedUtil::AddDebugLog(skCrypt("Failed to get module file name, error code: %d"), GetLastError());
        strTitleStatus = skCrypt("Agent Installation Failed");
        strMessageStatus = skCrypt("Failed to determine the installation path for the AtomicShield Agent. Please try again.");
        return;
    }

    std::string strAgentFilename(szAgentPath);
    size_t      pos = strAgentFilename.find_last_of("\\/");
    if (pos != std::string::npos)
        strAgentFilename = strAgentFilename.substr(pos + 1);
    
    int iRenameStatus = std::rename(szAgentPath, (std::string(szAgentPath) + ".old-" + SharedUtil::GenerateRandomString(4)).c_str());
    if (iRenameStatus != 0)
    {
        SharedUtil::AddDebugLog(skCrypt("Failed to rename existing agent file, error code: %d"), GetLastError());
        strTitleStatus = skCrypt("Agent Installation Failed");
        strMessageStatus = skCrypt("Failed to rename the existing AtomicShield Agent file. Please check your permissions and try again.");
        return;
    }

    FILE* pFile = fopen(szAgentPath, "wb");
    if (!pFile)
    {
        SharedUtil::AddDebugLog(skCrypt("Failed to open file for writing: %s, error code: %d"), szAgentPath, GetLastError());
        strTitleStatus = skCrypt("Agent Installation Failed");
        strMessageStatus = skCrypt("Failed to open the installation file for the AtomicShield Agent. Please check your permissions and try again.");
        return;
    }
    size_t written = fwrite(AgentBuffer.data(), 1, AgentBuffer.length(), pFile);
    fclose(pFile);
    
    SharedUtil::AddDebugLog(skCrypt("Successfully installed AtomicShield Agent to %s"), szAgentPath);
    
    strTitleStatus = skCrypt("AtomicShield Agent Installed");
    strMessageStatus = skCrypt("The AtomicShield Agent has been successfully installed. Please restart the application to apply changes.");

    // Open the agent executable as admin and pass the old agent path as an argument
    ShellExecute(NULL, "runas", szAgentPath, (std::string("--old ") + std::string(szAgentPath) + ".old").c_str(), NULL, SW_SHOWNORMAL);
    exit(0);
}
