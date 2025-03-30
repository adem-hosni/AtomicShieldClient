#include "StdInc.h"
#include <Psapi.h>

CProcessGuard::CProcessGuard()
{
}

CProcessGuard::~CProcessGuard()
{
}

std::string GetProcessPath(DWORD pid)
{
    std::string strProcessPath;
    HANDLE      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess)
    {
        char szProcessPath[MAX_PATH];
        if (GetModuleFileNameEx(hProcess, nullptr, szProcessPath, MAX_PATH))
        {
            strProcessPath = szProcessPath;
        }
        else
        {
            SharedUtil::AddDebugLog("GetModuleBaseName failed with error %d @Process::GetProcessName", GetLastError());
        }
    }
    else
    {
        SharedUtil::AddDebugLog("OpenProcess failed with error %d @  Process::GetProcessName", GetLastError());
    }
    CloseHandle(hProcess);

    return strProcessPath;
}

std::vector<Handles::SYSTEM_HANDLE> Handles::GetHandles()
{
    ULONG    bufferSize = 0x10000;
    PVOID    buffer = nullptr;
    NTSTATUS status = 0;

    do
    {
        buffer = malloc(bufferSize);
        if (!buffer)
        {
            SharedUtil::AddDebugLog("Memory allocation failed @ Handles::GetHandles");
            return {};
        }

        status = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)16, buffer, bufferSize, &bufferSize);
        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            free(buffer);
            bufferSize *= 2;
        }
        else if (!(((NTSTATUS)(status)) >= 0))
        {
            SharedUtil::AddDebugLog("NtQuerySystemInformation failed @ Handles::GetHandles");
            free(buffer);
            return {};
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    PSYSTEM_HANDLE_INFORMATION handleInfo = (PSYSTEM_HANDLE_INFORMATION)buffer;
    std::vector<SYSTEM_HANDLE> handles(handleInfo->Handles, handleInfo->Handles + handleInfo->HandleCount);
    free(buffer);
    return handles;
}

std::vector<Handles::SYSTEM_HANDLE> Handles::DetectOpenHandlesToFiveM()
{
    DWORD                               targetProcessId = g_pAtomicAntiCheat->GetProcessID();
    auto                                handles = GetHandles();
    std::vector<Handles::SYSTEM_HANDLE> handlesToFiveM;

    for (auto& handle : handles)
    {
        if (handle.ProcessId == 0 || handle.ProcessId == 4 || handle.ProcessId == GetCurrentProcessId())
        {
            continue;
        }

        HANDLE processHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, handle.ProcessId);
        if (processHandle)
        {
            HANDLE duplicatedHandle = INVALID_HANDLE_VALUE;

            if (DuplicateHandle(processHandle, (HANDLE)handle.Handle, GetCurrentProcess(), &duplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
            {
                if (GetProcessId(duplicatedHandle) == targetProcessId)
                {
                    handle.RefrencingFivem = true;
                    handlesToFiveM.push_back(handle);
                }
                else
                {
                    handle.RefrencingFivem = false;
                }

                if (duplicatedHandle != INVALID_HANDLE_VALUE)
                    CloseHandle(duplicatedHandle);
            }

            CloseHandle(processHandle);
        }
    }
    return handlesToFiveM;
}


void CProcessGuard::DoPulse()
{
    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
        {
            Sleep(50);
        }

        std::vector<Handles::SYSTEM_HANDLE> handles = Handles::DetectOpenHandlesToFiveM();

        for (auto& handle : handles)
        {
            std::string strProcessPath = GetProcessPath(handle.ProcessId);
            std::string strProcessName = Utils::ParseModuleNameFromPath(strProcessPath);

            bool bIsWhitelisted = false;

            for (int i = 0; i < std::size(Handles::Whitelisted); i++)
            {
                if (strcmp(Handles::Whitelisted[i], strProcessName.c_str()) == 0)
                {
                    bIsWhitelisted = true;
                    break;
                }
            }

            if (strProcessName.find("FiveM") != std::string::npos)
            {
                bIsWhitelisted = true;
            }

            if (bIsWhitelisted)
                continue;

            if (strProcessPath.find("C:\\Windows") != std::string::npos)
                continue;

            if (!strProcessPath.empty() &&
                (handle.GrantedAccess & PROCESS_ALL_ACCESS || handle.GrantedAccess & PROCESS_VM_WRITE || handle.GrantedAccess & PROCESS_VM_READ ||
                 handle.GrantedAccess & PROCESS_SUSPEND_RESUME || handle.GrantedAccess & PROCESS_SET_INFORMATION ||
                 handle.GrantedAccess & PROCESS_VM_OPERATION || handle.GrantedAccess & PROCESS_DUP_HANDLE))
            {
                SharedUtil::AddDebugLog("The Process %s with pid %d is opening FiveM.exe!", strProcessName.c_str(), handle.ProcessId);
                g_pAtomicAntiCheat->NotifyDetection(
                    MALICIOUS_PROCESS_HANDLE_OPEN,
                    {{"process_name", strProcessName}, {"process_path", strProcessPath}, {"pid", handle.ProcessId}, {"granted_access", handle.GrantedAccess}});
                    g_pAtomicAntiCheat->RunScanners(false);

            }
        }
    }
}
