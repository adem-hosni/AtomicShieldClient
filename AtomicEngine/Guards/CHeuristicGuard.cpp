#include <fstream>
#include "StdInc.h"
#include "KernelCalls.hpp"

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

std::vector<unsigned char> readBytesFromExe(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    if (!file)
    {
        SharedUtil::AddDebugLog("Unable to open the file!");
        return {};
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> rawData(fileSize);

    file.read(reinterpret_cast<char*>(rawData.data()), fileSize);

    file.close();

    return rawData;
}

std::string CHeuristicGuard::BuildSignatureParameters()
{
    std::stringstream ss;
    ss << (char)(m_Signatures.size() + 1);

    for (const std::string& param : m_Signatures)
    {
        ss << static_cast<char>(param.length() + 1);
        ss << param.c_str();
    }
    return ss.str();
}

void CHeuristicGuard::Initialize()
{
}

void CHeuristicGuard::SpawnScanProcess()
{
    const char* szFilePath = "C:\\Users\\hosni\\Desktop\\memscn-main\\src\\x64\\Release\\scn.exe";

    char szCommandLine[512];
    memset(szCommandLine, 0, sizeof(szCommandLine));
    sprintf(szCommandLine, "\"%s\" --pid %d --sigs %s", szFilePath, GetCurrentProcessId(), BuildSignatureParameters().c_str());

    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        SharedUtil::AddDebugLog("Failed to create pipe. Error: 0x%llx", GetLastError());
    }

    STARTUPINFOA        startupInfo = {0};
    PROCESS_INFORMATION processInfo = {0};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.hStdOutput = hWritePipe;
    startupInfo.hStdError = hWritePipe;
    startupInfo.hStdInput = NULL;

    if (!CreateProcess(nullptr,                     // Path to the executable
                       szCommandLine,               // Command line arguments (nullptr if none)
                       nullptr,                     // Process security attributes
                       nullptr,                     // Thread security attributes
                       FALSE,                       // Inherit handles
                       CREATE_NO_WINDOW,            // Creation flags
                       nullptr,                     // Use parent's environment block
                       nullptr,                     // Use parent's starting directory
                       &startupInfo,                // Pointer to STARTUPINFO structure
                       &processInfo))
    {            // Pointer to PROCESS_INFORMATION structure
        SharedUtil::AddDebugLog("Failed to create process. Error: 0x%llx", GetLastError());
    }

    CloseHandle(hWritePipe);

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    DWORD dwBytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &dwBytesRead, NULL) && dwBytesRead > 0)
    {
        buffer[dwBytesRead] = '\0';            // Null-terminate the buffer
        std::cout << buffer;                   // Print the output
    }
    SharedUtil::AddDebugLog("Buffer: %s", buffer);

    // Get the exit code
    DWORD dwExitCode = 0;
    if (GetExitCodeProcess(processInfo.hProcess, &dwExitCode))
    {
        SharedUtil::AddDebugLog("Process finished with exit code: 0x%llx", dwExitCode);

        if (dwExitCode == 0x1c8)
        {
            g_pSafeAntiCheat->NotifyDetection(eDetectionType::CHEAT_SIGNATURE_FOUND);
        }
    }
    else
    {
        SharedUtil::AddDebugLog("Failed to get exit code. Error: 0x%llx", GetLastError());
    }

    CloseHandle(hReadPipe);
    TerminateProcess(processInfo.hProcess, dwExitCode);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_Signatures.insert(Signature);
        }
    }

    // SpawnScanProcess();
}

inline static void __fastcall rpm( const std::vector<DWORD>& pids = std::vector<DWORD>())
{
    DWORD p[1024];
    DWORD n, j;

    std::wstring      memoryString = L"Dear ImGui Demo";
    std::wstring_view wstr(memoryString.begin(), memoryString.end());

    const DWORD  currentProcessId = GetCurrentProcessId();
    const HANDLE currentProcess = GetCurrentProcess();

    //for (DWORD i = 0; i < j; i++)
    {
        DWORD targetProcessId = GetCurrentProcessId();

        NTSTATUS                      status;
        KernelCalls_OBJECT_ATTRIBUTES objAttr{};
        KernelCalls_CLIENT_ID         clientId{};
        HANDLE                        processHandle = GetCurrentProcess();

        RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
        objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
        RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));
        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(targetProcessId));

        /*status = SysNtOpenProcess(&processHandle, (0x0400) | (0x0010), &objAttr, &clientId);
        if (!NT_SUCCESS(status))
            continue;*/

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        MEMORY_BASIC_INFORMATION memoryInfo{};
        bool                     found = false;

        for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
             addr = static_cast<LPBYTE>(memoryInfo.BaseAddress) + memoryInfo.RegionSize)
        {
            PVOID  baseAddress = addr;
            SIZE_T regionSize = sizeof(memoryInfo);
            SIZE_T returnLength;

            status = SysNtQueryVirtualMemory(processHandle, baseAddress, MemoryBasicInformation, &memoryInfo, regionSize, &returnLength);
            if (!NT_SUCCESS(status) || memoryInfo.State != MEM_COMMIT || memoryInfo.Protect & PAGE_NOACCESS)
                continue;

            SIZE_T allocationSize = memoryInfo.RegionSize + wstr.size() * sizeof(wchar_t) - 1;            // Extra space for overlap
            PVOID  buffer = nullptr;
            status = SysNtAllocateVirtualMemory(currentProcess, &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!NT_SUCCESS(status))
                continue;

            SIZE_T bytesRead = 0;
            status = SysNtReadVirtualMemory(processHandle, memoryInfo.BaseAddress, buffer, memoryInfo.RegionSize, &bytesRead);
            if (NT_SUCCESS(status))
            {

                const wchar_t* dataPtr = reinterpret_cast<const wchar_t*>(buffer);
                size_t         wordCount = bytesRead / sizeof(wchar_t);

                size_t foundPos = std::wstring_view(dataPtr, wordCount).find(wstr);

                if (foundPos != std::wstring_view::npos)
                {
                    found = true;
                    LPVOID lpFlaggedAddress = static_cast<LPBYTE>(memoryInfo.BaseAddress) + foundPos * sizeof(wchar_t);
                    if ((DWORD64)lpFlaggedAddress != (DWORD64)memoryString.data())
                    {
                        SharedUtil::AddDebugLog("Found at 0x%p | 0x%p", lpFlaggedAddress, (DWORD64)memoryString.data());
                    }
                    break;
                }
            }

            SysNtFreeVirtualMemory(currentProcess, &buffer, &allocationSize, MEM_RELEASE);
            if (found)
                break;
        }

        SysNtClose(processHandle);
    }
}

void CHeuristicGuard::DoPulse()
{
    while (true)
    {
        std::vector<DWORD> pids = {GetCurrentProcessId()};

        
        LARGE_INTEGER     frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        rpm( pids);

        QueryPerformanceCounter(&end);
        float elapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        SharedUtil::AddDebugLog("[+] Scan completed in %.2f .s");

        Sleep(500);
    }
}