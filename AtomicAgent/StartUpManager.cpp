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



bool StartupManager::IsAppInTaskScheduler()
{
    HRESULT          hr = S_OK;
    ITaskService*    pService = nullptr;
    ITaskFolder*     pRootFolder = nullptr;
    IRegisteredTask* pRegisteredTask = nullptr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("COM initialization failed: 0x%08X", hr);
        return false;
    }

    hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Task Scheduler creation failed: 0x%08X", hr);
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Task Scheduler connection failed: 0x%08X", hr);
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Getting root folder failed: 0x%08X", hr);
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pRootFolder->GetTask(_bstr_t(L"AtomicShield"), &pRegisteredTask);

    bool exists = false;
    if (SUCCEEDED(hr) && pRegisteredTask != nullptr)
    {
        TASK_STATE taskState;
        hr = pRegisteredTask->get_State(&taskState);
        if (SUCCEEDED(hr) && taskState != TASK_STATE_DISABLED)
        {
            exists = true;
        }
    }

    if (pRegisteredTask)
        pRegisteredTask->Release();
    if (pRootFolder)
        pRootFolder->Release();
    if (pService)
        pService->Release();

    CoUninitialize();

    return exists;
}

bool StartupManager::AddAppToTaskScheduler()
{
    HRESULT             hr = S_OK;
    ITaskService*       pService = nullptr;
    ITaskFolder*        pRootFolder = nullptr;
    IRegisteredTask*    pRegisteredTask = nullptr;
    ITaskDefinition*    pTask = nullptr;
    IRegistrationInfo*  pRegInfo = nullptr;
    IPrincipal*         pPrincipal = nullptr;
    ITaskSettings*      pSettings = nullptr;
    ITriggerCollection* pTriggerCollection = nullptr;
    ITrigger*           pTrigger = nullptr;
    ILogonTrigger*      pLogonTrigger = nullptr;
    IActionCollection*  pActionCollection = nullptr;
    IAction*            pAction = nullptr;
    IExecAction*        pExecAction = nullptr;

    // Get the executable path and separate path from arguments
    char szPath[MAX_PATH];
    GetModuleFileNameA(nullptr, szPath, MAX_PATH);

    // Path should be in quotes, arguments separate
    std::string exePath = "\"" + std::string(szPath) + "\"";
    std::string arguments = "--startup";

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("COM initialization failed: 0x%08X", hr);
        return false;
    }

    hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Task Scheduler creation failed: 0x%08X", hr);
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Task Scheduler connection failed: 0x%08X", hr);
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Getting root folder failed: 0x%08X", hr);
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Check if task already exists and delete it first
    hr = pRootFolder->GetTask(_bstr_t(L"AtomicShield"), &pRegisteredTask);
    if (SUCCEEDED(hr) && pRegisteredTask != nullptr)
    {
        pRootFolder->DeleteTask(_bstr_t(L"AtomicShield"), 0);
        pRegisteredTask->Release();
        pRegisteredTask = nullptr;
    }

    // Create the task definition
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Creating new task failed: 0x%08X", hr);
        goto cleanup;
    }

    // Registration info
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr))
        goto cleanup;

    hr = pRegInfo->put_Author(_bstr_t(L"AtomicShield"));
    if (FAILED(hr))
        goto cleanup;
    hr = pRegInfo->put_Description(_bstr_t(L"AtomicShield startup task"));
    if (FAILED(hr))
        goto cleanup;

    // Principal with highest privileges
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr))
        goto cleanup;

    hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    if (FAILED(hr))
        goto cleanup;
    hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    if (FAILED(hr))
        goto cleanup;

    // Settings
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr))
        goto cleanup;

    hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
    if (FAILED(hr))
        goto cleanup;
    hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
    if (FAILED(hr))
        goto cleanup;
    hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
    if (FAILED(hr))
        goto cleanup;
    hr = pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));            // No time limit
    if (FAILED(hr))
        goto cleanup;
    hr = pSettings->put_AllowHardTerminate(VARIANT_FALSE);
    if (FAILED(hr))
        goto cleanup;

    // Trigger - run at logon
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr))
        goto cleanup;

    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    if (FAILED(hr))
        goto cleanup;

    hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
    if (FAILED(hr))
        goto cleanup;

    hr = pLogonTrigger->put_Id(_bstr_t(L"LogonTrigger"));
    if (FAILED(hr))
        goto cleanup;
    hr = pLogonTrigger->put_Delay(_bstr_t(L"PT30S"));            // 30 second delay
    if (FAILED(hr))
        goto cleanup;

    // Action - execute the application with separate path and arguments
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr))
        goto cleanup;

    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    if (FAILED(hr))
        goto cleanup;

    hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    if (FAILED(hr))
        goto cleanup;

    // Set path (in quotes) and arguments separately
    hr = pExecAction->put_Path(_bstr_t(exePath.c_str()));
    if (FAILED(hr))
        goto cleanup;

    hr = pExecAction->put_Arguments(_bstr_t(arguments.c_str()));
    if (FAILED(hr))
        goto cleanup;

    // Register the task
    hr = pRootFolder->RegisterTaskDefinition(_bstr_t(L"AtomicShield"), pTask, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN,
                                             _variant_t(L""), &pRegisteredTask);

    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Failed to register task: 0x%08X", hr);
        goto cleanup;
    }

    SharedUtil::AddDebugLog("Startup added to Task Scheduler successfully");

cleanup:
    if (pExecAction)
        pExecAction->Release();
    if (pAction)
        pAction->Release();
    if (pActionCollection)
        pActionCollection->Release();
    if (pLogonTrigger)
        pLogonTrigger->Release();
    if (pTrigger)
        pTrigger->Release();
    if (pTriggerCollection)
        pTriggerCollection->Release();
    if (pSettings)
        pSettings->Release();
    if (pPrincipal)
        pPrincipal->Release();
    if (pRegInfo)
        pRegInfo->Release();
    if (pTask)
        pTask->Release();
    if (pRegisteredTask)
        pRegisteredTask->Release();
    if (pRootFolder)
        pRootFolder->Release();
    if (pService)
        pService->Release();

    CoUninitialize();

    return SUCCEEDED(hr);
}
bool StartupManager::RemoveAppFromTaskScheduler()
{
    HRESULT       hr = S_OK;
    ITaskService* pService = nullptr;
    ITaskFolder*  pRootFolder = nullptr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("COM initialization failed: 0x%08X", hr);
        return false;
    }

    hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Task Scheduler creation failed: 0x%08X", hr);
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Task Scheduler connection failed: 0x%08X", hr);
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Getting root folder failed: 0x%08X", hr);
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pRootFolder->DeleteTask(_bstr_t(L"AtomicShield"), 0);

    bool success = SUCCEEDED(hr);
    if (success)
    {
        SharedUtil::AddDebugLog("Startup removed from Task Scheduler");
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        SharedUtil::AddDebugLog("Task not found in Task Scheduler");
        success = true;            // Consider task not found as successful removal
    }
    else
    {
        SharedUtil::AddDebugLog("Failed to remove task from Task Scheduler: 0x%08X", hr);
    }

    if (pRootFolder)
        pRootFolder->Release();
    if (pService)
        pService->Release();

    CoUninitialize();

    return success;
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