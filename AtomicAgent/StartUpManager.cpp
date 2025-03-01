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
        std::wcerr << L"Error retrieving executable path" << std::endl;
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

void StartupManager::StartupFunction()
{
    static bool        bDownloading = false;
    static std::string strEngineBuffer;
    static bool        bInjected = false;
    static char        szLoadingMessage[144];
    static SUserData   DownloadData{};



    //MessageBoxA(NULL, "Running in startup mode", "Startup", MB_OK);

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
        int iProcessID = SharedUtil::GetFivemProcessID();

        if (iProcessID > 0 && GUI::isFiveMReady())
        {
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, iProcessID);
            if (hProcess)
            {
                char szTempFilePath[MAX_PATH];
                GetTempPathA(MAX_PATH, szTempFilePath);
                sprintf(szTempFilePath, "%s%s.dll", szTempFilePath, SharedUtil::GenerateRandomString(32).c_str());

                FILE* file = fopen(szTempFilePath, "wb");
                if (file)
                {
                    fwrite(strEngineBuffer.c_str(), sizeof(char), strEngineBuffer.size(), file);
                    fclose(file);
                }

                bInjected = GUI::InjectDLL(hProcess, iProcessID, szTempFilePath);
                if (bInjected)
                {

                    MessageBoxA(NULL, "Have fun!", "Atomic Shield", MB_OK);
                    std::remove(szTempFilePath);
                    page = 2;
                    active_anim_1 = true;
                }
                else
                {
                    MessageBoxA(NULL, "An error occurred while loading!", "Failed", MB_ICONERROR);
                }
                std::remove(szTempFilePath);
                CloseHandle(hProcess);
            }
            else
            {
                SharedUtil::AddDebugLog("Failed to get process handle!\n");
            }
        }
        else
        {
            SharedUtil::AddDebugLog("Waiting for FiveM to launch...");
            std::this_thread::sleep_for(std::chrono::seconds(10));            
        }
    }
    
}
