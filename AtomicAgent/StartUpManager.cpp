#include <windows.h>
#include <comdef.h>
#include <taskschd.h>
#include <atlbase.h>
#include <atlcom.h>
#include <iostream>
#include "StdInc.h"
#include <EngineLauncher.h>
#include <GUI/GUI.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "shlwapi.lib")

bool StartupManager::IsAppInRegistry()
{
    HKEY    hKey;
    LSTATUS result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);

    if (result == ERROR_SUCCESS)
    {
        char  value[1024];
        DWORD size = sizeof(value);
        result = RegQueryValueExA(hKey, "AtomicShield", nullptr, nullptr, (LPBYTE)value, &size);
        RegCloseKey(hKey);

        return result == ERROR_SUCCESS;
    }

    return false;
}
bool StartupManager::AddAppToRegistry()
{
    HKEY    hKey;
    LSTATUS result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);

    if (result == ERROR_SUCCESS)
    {
        char szPath[MAX_PATH];
        GetModuleFileNameA(nullptr, szPath, MAX_PATH);

        // Add the --startup argument
        std::string fullPath = std::string(szPath) + " --startup";

        result = RegSetValueExA(hKey, "AtomicShield", 0, REG_SZ, (const BYTE*)fullPath.c_str(), fullPath.length() + 1);

        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS)
        {
            SharedUtil::AddDebugLog("Startup added to registry successfully");
            return true;
        }
    }

    SharedUtil::AddDebugLog("Failed to add startup registry: %d", result);
    return false;
}
bool StartupManager::RemoveAppFromRegistry()
{
    HKEY    hKey;
    LSTATUS result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);

    if (result == ERROR_SUCCESS)
    {
        result = RegDeleteValueA(hKey, "AtomicShield");
        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
        {
            SharedUtil::AddDebugLog("Startup removed from registry");
            return true;
        }
    }

    SharedUtil::AddDebugLog("Failed to remove startup registry: %d", result);
    return false;
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
            SharedUtil::SetRegistryIntValue("AtomicShield", "AtomicShield", 0);
        }
    }
    else
    {
        SharedUtil::AddDebugLog("[Startup] Load failed: %d, error: 0x%llX", iLoadResult, lastErr);
    }
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