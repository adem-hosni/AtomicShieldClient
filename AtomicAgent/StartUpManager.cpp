#include <iostream>
#include "StdInc.h">
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

bool StartupManager::IsAppInRegistry(std::string& appName)
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType = 0;
        DWORD dwSize = MAX_PATH;
        WCHAR szValue[MAX_PATH] = {0};

        if (RegQueryValueEx(hKey, appName.c_str(), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }

        RegCloseKey(hKey);
    }
    return false;
}

bool StartupManager::AddAppToRegistry(std::string& appName)
{
    HKEY        hKey;
    std::string appPath = "\"";

    char szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        std::wcerr << L"Error retrieving executable path" << std::endl;
        return false;
    }

    appPath += szPath;
    appPath += "\" --startup";

    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hKey, appName.c_str(), 0, REG_SZ, (const BYTE*)appPath.c_str(), (appPath.length() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }
    return false;
}

std::string StartupManager::GetCurrentProcessName()
{
    char szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        SharedUtil::AddDebugLog("Error retrieving executable path");
        return "";
    }
    std::string fileName = PathFindFileName(szPath);
    return fileName;
}

void StartupManager::StartupFunction(bool bNoErrors, std::string strErrorTitle, std::string strErrorDescription)
{
    static bool        bDownloading = false;
    static std::string strAgentPEBBuffer;
    static bool        bInjected = false;
    static char        szLoadingMessage[144];
    // if (!bNoErrors)
    //{
    //     MessageBoxA(NULL, "Running in startup mode", NULL, NULL);
    //     std::thread AgentPEBDownloader(
    //         [&]()
    //         {
    //             g_pAtomicAPI->DownloadEngine(&strAgentPEBBuffer);
    //             bDownloading = false;
    //         });

    //    AgentPEBDownloader.detach();
    //    bDownloading = true;

    //    while (bDownloading || strAgentPEBBuffer.empty())
    //    {
    //        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //    }

    //    if (!strAgentPEBBuffer.empty() && !bInjected)
    //    {
    //        int iProcessID = SharedUtil::GetProcessID("Notepad.exe");
    //        SharedUtil::AddDebugLog("Process ID retrieved: %d", iProcessID);

    //        if (iProcessID > 0)
    //        {
    //            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, iProcessID);
    //            if (hProcess)
    //            {
    //                memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
    //                strcat(szLoadingMessage, "Please wait, we're getting everything ready for you!");
    //                bInjected = ManualMapDll(hProcess, reinterpret_cast<BYTE*>((char*)strAgentPEBBuffer.c_str()), strAgentPEBBuffer.size());
    //                if (bInjected)
    //                    __fastfail(0);
    //                SharedUtil::AddDebugLog("Result from dll injection: %d (0x%x)", bInjected, GetLastError());
    //                bInjected = true;
    //            }
    //            else
    //            {
    //                SharedUtil::AddDebugLog("Failed to get process handle!\n");
    //            }
    //        }
    //        else
    //        {
    //            SharedUtil::AddDebugLog("waiting");
    //            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
    //            strcat(szLoadingMessage, "Waiting for FiveM to launch");
    //        }
    //    }
    // }
    // else
    //{
    //    MessageBoxA(NULL, strErrorDescription.c_str(), strErrorTitle.c_str(), MB_OK | MB_ICONINFORMATION);

    //}
}
