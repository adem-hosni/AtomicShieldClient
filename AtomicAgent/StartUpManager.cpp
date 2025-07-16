#include <iostream>
#include "StdInc.h">
#include <shlwapi.h>
#include <EngineLauncher.h>
#include <GUI/GUI.h>
#pragma comment(lib, "shlwapi.lib")

bool StartupManager::IsAppInRegistry()
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, skCrypt("Software\\Microsoft\\Windows\\CurrentVersion\\Run").decrypt(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType = 0;
        DWORD dwSize = MAX_PATH;
        WCHAR szValue[MAX_PATH] = {0};

        if (RegQueryValueEx(hKey, "AtomicShield", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }

        RegCloseKey(hKey);
    }
    return false;
}

bool StartupManager::AddAppToRegistry()
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
        if (RegSetValueEx(hKey, "AtomicShield", 0, REG_SZ, (const BYTE*)appPath.c_str(), (appPath.length() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }
    return false;
}

bool StartupManager::RemoveAppFromRegistry()
{
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, skCrypt("Software\\Microsoft\\Windows\\CurrentVersion\\Run").decrypt(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        // Try to delete the value
        LONG result = RegDeleteValue(hKey, "AtomicShield");
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
    static bool        bDownloadStarted = false;
    static bool        bDownloadFinished = false;
    static bool        bInjected = false;
    static std::string strEngineBuffer;
    static char        szLoadingMessage[144] = {0};
    static SUserData   DownloadData{};

    std::thread downloader(
        []()
        {
            bDownloadStarted = true;        
            g_pAtomicAPI->DownloadEngine(&strEngineBuffer, &DownloadData);
            bDownloadFinished = true;
        });
    downloader.detach();

    while (!bDownloadFinished)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (strEngineBuffer.empty())
    {
        SharedUtil::AddDebugLog("[Startup] Engine buffer is empty!");
        return;
    }

    if (SharedUtil::GetProcessID(skCrypt("AtomicSvc.exe")) != NULL)
    {
        SharedUtil::AddDebugLog("[Startup] AtomicSvc.exe already running. Skipping startup loading.");
        return;
    }

    std::filesystem::path EnginePath = EngineLauncher::GetEnginePath();
    if (!EngineLauncher::DumpEngineProcess(EnginePath, EngineLauncher::pProcessBuffer, sizeof(EngineLauncher::pProcessBuffer)))
    {
        SharedUtil::AddDebugLog("[Startup] Failed to dump engine process to disk.");
        return;
    }

    HANDLE                        hLauncher = INVALID_HANDLE_VALUE;
    EngineLauncher::eLaunchResult result = EngineLauncher::LaunchEngineProcess(EnginePath, &hLauncher);
    if (result != EngineLauncher::eLaunchResult::SUCCESS || hLauncher == INVALID_HANDLE_VALUE)
    {
        SharedUtil::AddDebugLog("[Startup] Failed to launch engine process. Result: %d", result);
        return;
    }

    int iLoadResult = EngineLauncher::LoadEngineIntoLauncher(EnginePath, hLauncher, reinterpret_cast<BYTE*>(const_cast<char*>(strEngineBuffer.c_str())),
                                                                    strEngineBuffer.size());

    DWORD lastErr = GetLastError();
    SharedUtil::AddDebugLog("[Startup] Loading result: %d, LastError: 0x%llX", iLoadResult, lastErr);

    if (iLoadResult == 0 && lastErr == ERROR_SUCCESS)
    {
        bool   bFailure = false;
        time_t startTime = time(NULL);
        while (!CheckIfLoaded("Software\\AtomicShield"))
        {
            if (time(NULL) - startTime > 5)
            {
                bFailure = true;
                break;
            }
            Sleep(90);
        }

        if (bFailure)
        {
            SharedUtil::AddDebugLog("[Startup] Loading timed out, module may be blocked.");
        }
        else
        {
            SharedUtil::AddDebugLog("[Startup] Engine loaded successfully.");
            SharedUtil::SetRegistryIntValue("AtomicShield", 0);
        }
    }
    else
    {
        SharedUtil::AddDebugLog("[Startup] Load failed: %d, error: 0x%llX", iLoadResult, lastErr);
    }
}
