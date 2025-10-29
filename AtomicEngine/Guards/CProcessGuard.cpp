#include "StdInc.h"
#include <winternl.h>
#include <Psapi.h>

CProcessGuard::CProcessGuard()
{
    m_vDetectedProcesses = {};
    m_ullLastHeartbeat = NULL;
    m_ullLastPulse = NULL;
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
            PROCESS_LOG("GetModuleBaseName failed with error %d @Process::GetProcessName", GetLastError());
        }
    }
    else
    {
        PROCESS_LOG("OpenProcess failed with error %d @  Process::GetProcessName", GetLastError());
    }
    CloseHandle(hProcess);

    return strProcessPath;
}

std::vector<SYSTEM_HANDLE> CProcessGuard::GetHandles()
{
    ULONG    bufferSize = 0x10000;
    PVOID    buffer = nullptr;
    NTSTATUS status = 0;

    do
    {
        buffer = malloc(bufferSize);
        if (!buffer)
        {
            PROCESS_LOG("Memory allocation failed @ GetHandles");
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
            PROCESS_LOG("NtQuerySystemInformation failed @ GetHandles");
            free(buffer);
            return {};
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    PSYSTEM_HANDLE_INFORMATION handleInfo = (PSYSTEM_HANDLE_INFORMATION)buffer;
    std::vector<SYSTEM_HANDLE> handles(handleInfo->Handles, handleInfo->Handles + handleInfo->HandleCount);
    free(buffer);
    return handles;
}

std::vector<SYSTEM_HANDLE> CProcessGuard::DetectOpenHandlesToFiveM()
{
    DWORD                               targetProcessId = g_pAtomicAntiCheat->GetProcessID();
    auto                                handles = GetHandles();
    std::vector<SYSTEM_HANDLE> handlesToFiveM;

    for (auto& handle : handles)
    {
        if (handle.ProcessId == 0 || handle.ProcessId == 4 || handle.ProcessId == GetCurrentProcessId())
        {
            continue;
        }

        m_ullLastHeartbeat = Utils::FastEpochSeconds();

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
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (!g_pAtomicAntiCheat->IsValidProcessHandle())
            std::this_thread::sleep_for(std::chrono::seconds(1));

        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        m_ullLastHeartbeat = Utils::FastEpochSeconds();

        if (!(Utils::FastEpochSeconds() - m_ullLastPulse > 15 || m_ullLastPulse == NULL))
            continue;

        m_ullLastPulse = m_ullLastHeartbeat;

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        std::vector<SYSTEM_HANDLE> handles = DetectOpenHandlesToFiveM();

        for (auto& handle : handles)
        {
            m_ullLastHeartbeat = Utils::FastEpochSeconds();
            std::string strProcessPath = GetProcessPath(handle.ProcessId);
            std::string strProcessName = Utils::ParseModuleNameFromPath(strProcessPath);

            if (strProcessPath.empty())
                continue;

            if (strProcessPath.find("C:\\Windows") != std::string::npos)
                continue;

            SharedUtil::AddDebugLog("[ProcessGuard] Handle opened: %s (PID: %d, Access: 0x%X)", strProcessPath.c_str(), handle.ProcessId, handle.GrantedAccess);

            if (!(handle.GrantedAccess & (PROCESS_ALL_ACCESS | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_SUSPEND_RESUME | PROCESS_SET_INFORMATION |
                                          PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION)))
                continue;

            if (!FileAuthentication::HasSignature(strProcessPath))
            {
                if (std::find(m_vDetectedProcesses.begin(), m_vDetectedProcesses.end(), strProcessPath) == m_vDetectedProcesses.end())
                {
                    std::string strFileHash = Utils::GetFileHash(strProcessPath);
                    SharedUtil::AddDebugLog("[ProcessGuard] Detected malicious process : % s(PID : % d, Granted Access : 0x % X, Hash: %s) ",
                                            strProcessPath.c_str(), handle.ProcessId, handle.GrantedAccess, strFileHash.c_str());

                    m_vDetectedProcesses.push_back(strProcessPath);

                    g_pAtomicAntiCheat->NotifyDetection(MALICIOUS_PROCESS_HANDLE_OPEN, {{"process_name", strProcessName},
                                                                                        {"process_path", strProcessPath},
                                                                                        {"hash", strFileHash},
                                                                                        {"pid", handle.ProcessId},
                                                                                        {"granted_access", handle.GrantedAccess}});
                    g_pAtomicAntiCheat->GetNetwork()->RequestFileUpload(strProcessPath, strFileHash);
                }
            }
        }
        QueryPerformanceCounter(&end);

        m_ullLastHeartbeat = Utils::FastEpochSeconds();

        float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        SharedUtil::AddDebugLog("[ProcessGuard] Process Guard Pulse completed in %.5f seconds", fElapsedTime);

        //std::this_thread::sleep_for(std::chrono::seconds(15));
    }

    _endthreadex(0);
}

void CProcessGuard::ClearDetections()
{
    m_vDetectedProcesses.clear();
    m_ullLastHeartbeat = NULL;
}