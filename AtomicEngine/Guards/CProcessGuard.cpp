#include "StdInc.h"
#include <Psapi.h>

CProcessGuard::CProcessGuard()
{
}

CProcessGuard::~CProcessGuard()
{
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

std::vector<Handles::SYSTEM_HANDLE> Handles::DetectOpenHandlesToProcess()
{
    DWORD                               currentProcessId = GetCurrentProcessId();
    auto                                handles = GetHandles();
    std::vector<Handles::SYSTEM_HANDLE> handlesTous;

    for (auto& handle : handles)
    {
        if (handle.ProcessId != currentProcessId)
        {
            if (handle.ProcessId == 0 || handle.ProcessId == 4)
            {
                continue;
            }

            HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, handle.ProcessId);

            if (processHandle)
            {
                HANDLE duplicatedHandle = INVALID_HANDLE_VALUE;

                if (DuplicateHandle(processHandle, (HANDLE)handle.Handle, GetCurrentProcess(), &duplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
                {
                    if (GetProcessId(duplicatedHandle) == currentProcessId)
                    {
                        handle.ReferencingOurProcess = true;
                        handlesTous.push_back(handle);
                    }
                    else
                    {
                        handle.ReferencingOurProcess = false;
                    }

                    if (duplicatedHandle != INVALID_HANDLE_VALUE)
                        CloseHandle(duplicatedHandle);
                }

                CloseHandle(processHandle);
            }
        }
    }
    return handlesTous;
}

bool Handles::DoesProcessHaveOpenHandleTous(DWORD pid, std::vector<Handles::SYSTEM_HANDLE> handles)
{
    if (pid == 0 || pid == 4)            // system idle process + system pids
        return false;

    for (const auto& handle : handles)
    {
        if (handle.ProcessId == pid && handle.ReferencingOurProcess)
        {
            return true;
        }
    }

    return false;
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

    return strProcessPath;
}

void CProcessGuard::DoPulse()
{
    while (true)
    {
        std::vector<Handles::_SYSTEM_HANDLE> handles = Handles::DetectOpenHandlesToProcess();
        bool                                 bFoundHandle = false;

        for (auto& handle : handles)
        {
            if (Handles::DoesProcessHaveOpenHandleTous(handle.ProcessId, handles))
            {
                std::string strProcessPath = GetProcessPath(handle.ProcessId);
                int         size = sizeof(Handles::Whitelisted) / sizeof(UINT64);

                bool        bIsWhitelisted = false;
                std::string strProcessName = Utils::ParseModuleNameFromPath(strProcessPath);

                for (int i = 0; i < size; i++)
                {
                    // if (!FileAuthentication::HasSignature(std::wstring(strProcessPath.begin(), strProcessPath.end()).c_str()))
                    {
                        if (strcmp(Handles::Whitelisted[i], strProcessName.c_str()) == 0)
                        {
                            bIsWhitelisted = true;
                        }
                    }
                }
                if (!bIsWhitelisted && !strProcessPath.empty())
                {
                    SharedUtil::AddDebugLog("The Process %s with pid %d is opening our process!", strProcessName.c_str(), handle.ProcessId);
                    g_pAtomicAntiCheat->NotifyDetection(MALICIOUS_PROCESS_HANDLE_OPEN, {{"process_name", strProcessName},
                                                                                        {"process_path", strProcessPath},
                                                                                        {"pid", handle.ProcessId},
                                                                                        {"granted_access", handle.GrantedAccess}});
                    bFoundHandle = TRUE;
                }
            }
            Sleep(60);
        }
    }
}
