#include "StdInc.h"
using namespace std;

CProcessGuard::CProcessGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
    m_dwMaxAddress = NULL;
}

CProcessGuard::~CProcessGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

std::vector<Handles::SYSTEM_HANDLE> Handles::GetHandles()
{
    NtQuerySystemInformationFunc NtQuerySystemInformation =
        (NtQuerySystemInformationFunc)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");
    if (!NtQuerySystemInformation)
    {
        std::cout << "Could not get NtQuerySystemInformation function address @ Handles::GetHandles";
        return {};
    }

    ULONG    bufferSize = 0x10000;
    PVOID    buffer = nullptr;
    NTSTATUS status = 0;

    do
    {
        buffer = malloc(bufferSize);
        if (!buffer)
        {
            std::cout << "Memory allocation failed @ Handles::GetHandles";
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
            std::cout << "NtQuerySystemInformation failed @ Handles::GetHandles";
            free(buffer);
            return {};
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    PSYSTEM_HANDLE_INFORMATION handleInfo = (PSYSTEM_HANDLE_INFORMATION)buffer;
    std::vector<SYSTEM_HANDLE> handles(handleInfo->Handles, handleInfo->Handles + handleInfo->HandleCount);
    free(buffer);
    return handles;
}

/*
    DetectOpenHandlesToProcess - returns a vector of SYSTEM_HANDLE which represent open handles in other processes to our current process
    Can be used to detect OpenProcess , a bit expensive on CPU though since all system handles must be enumerated
*/
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
                        std::cout << "Handle %d from process %d is referencing our process.", handle.Handle, handle.ProcessId;
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
            else
            {
                std::cout << "Couldn't open process with id %d @ Handles::DetectOpenHandlesToProcess (possible LOCALSERVICE or SYSTEM process)",
                    handle.ProcessId;
                continue;
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

std::wstring GetProcessName(DWORD pid)
{
    std::wstring processName;
    HANDLE       hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess)
    {
        HMODULE hMod;
        DWORD   cbNeeded;
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
        {
            WCHAR processNameBuffer[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, hMod, processNameBuffer, sizeof(processNameBuffer) / sizeof(WCHAR)))
            {
                processName = processNameBuffer;
            }
            else
            {
                // Logger::logf("UltimateAnticheat.log", Err, "GetModuleBaseName failed with error %d @  Process::GetProcessName", GetLastError());
            }
        }
        else
        {
            // Logger::logf("UltimateAnticheat.log", Err, "EnumProcessModules failed with error %d @  Process::GetProcessName", GetLastError());
        }
        CloseHandle(hProcess);
    }
    else
    {
        // Logger::logf("UltimateAnticheat.log", Err, "OpenProcess failed with error %d @  Process::GetProcessName", GetLastError());
    }

    return processName;
}

class Detections
{
public:
    static BOOL CheckOpenHandles()
    {
        BOOL                                 foundHandle = FALSE;
        std::vector<Handles::_SYSTEM_HANDLE> handles = Handles::DetectOpenHandlesToProcess();

        for (auto& handle : handles)
        {
            if (Handles::DoesProcessHaveOpenHandleTous(handle.ProcessId, handles))
            {
                std::wstring procName = GetProcessName(handle.ProcessId);
                int          size = sizeof(Handles::Whitelisted) / sizeof(UINT64);

                for (int i = 0; i < size; i++)
                {
                    if (wcscmp(Handles::Whitelisted[i], procName.c_str()) == 0)
                    {            // Whitelisted program has open handle
                        goto inner_break;
                    }
                }

                std::wcout << L"Detection: Process " << procName << L" has an open process handle to our process." << std::endl;
                foundHandle = TRUE;

            inner_break:
                continue;
            }
        }

        return foundHandle;
    }
};


void CProcessGuard::DoPulse()
{
        std::cout << "scanning";
        if (Detections::CheckOpenHandles())
        {
            std::cout << " Detected processes with open handles to this process.";
        }
        else
        {
            std::cout << " No suspicious handles detected.";
        }
}




