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
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return false;

    ITaskService* pService = nullptr;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr))
    {
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegisteredTask* pTask = nullptr;
    hr = pRootFolder->GetTask(_bstr_t(L"AtomicShield"), &pTask);

    bool result = SUCCEEDED(hr);

    if (pTask)
        pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    return result;
}

bool StartupManager::AddAppToRegistry()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return false;

    ITaskService* pService = nullptr;
    hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr) || !pService)
    {
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr) || !pRootFolder)
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Delete old task if exists
    pRootFolder->DeleteTask(_bstr_t(L"AtomicShield"), 0);

    ITaskDefinition* pTask = nullptr;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr) || !pTask)
    {
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Create logon trigger
    ITriggerCollection* pTriggerCollection = nullptr;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (SUCCEEDED(hr) && pTriggerCollection)
    {
        ITrigger* pTrigger = nullptr;
        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        if (pTrigger)
            pTrigger->Release();
        pTriggerCollection->Release();
    }

    // Action (start exe)
    IActionCollection* pActionCollection = nullptr;
    hr = pTask->get_Actions(&pActionCollection);
    if (SUCCEEDED(hr) && pActionCollection)
    {
        IAction* pAction = nullptr;
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        if (SUCCEEDED(hr) && pAction)
        {
            IExecAction* pExecAction = nullptr;
            hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
            if (SUCCEEDED(hr) && pExecAction)
            {
                char szPath[MAX_PATH];
                if (GetModuleFileNameA(nullptr, szPath, MAX_PATH) != 0)
                {
                    std::wstring wPath(szPath, szPath + strlen(szPath));

                    // Set path and args
                    pExecAction->put_Path(_bstr_t(szPath));
                    pExecAction->put_Arguments(_bstr_t(L"--startup"));
                }
                pExecAction->Release();
            }
            pAction->Release();
        }
        pActionCollection->Release();
    }

    // Register task
    IRegisteredTask* pRegisteredTask = nullptr;
    hr = pRootFolder->RegisterTaskDefinition(_bstr_t(L"AtomicShield"), pTask, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN,
                                             _variant_t(L""), &pRegisteredTask);

    if (pRegisteredTask)
        pRegisteredTask->Release();
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    return SUCCEEDED(hr);
}

bool StartupManager::RemoveAppFromRegistry()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return false;

    ITaskService* pService = nullptr;
    hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr) || !pService)
    {
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr) || !pRootFolder)
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pRootFolder->DeleteTask(_bstr_t(L"AtomicShield"), 0);

    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    return SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
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