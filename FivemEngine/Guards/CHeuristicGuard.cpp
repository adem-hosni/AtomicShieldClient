#include "StdInc.h"
#include <fstream>

std::unordered_set<std::string> m_Signatures;

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

std::string BuildSignatureParameters(std::unordered_set<std::string> params)
{
    params = {"dsl.wcsurmhfw.frp", "vxvdqr.uh"};
    std::stringstream ss;
    ss << (char)(params.size() + 1);
    for (auto it = params.begin(); it != params.end(); ++it)
    {
        ss << (char)((*it).length() + 1);
        ss << (*it).c_str();
    }
    return ss.str();
}

void CHeuristicGuard::Initialize()
{
    // CAtomicThread::Create(&SearchForString, reinterpret_cast<PVOID>(2));

    const char* szFilePath = "C:\\Users\\hosni\\Desktop\\memscn-main\\src\\x64\\Release\\scn.exe";

    char szCommandLine[512];
    memset(szCommandLine, 0, sizeof(szCommandLine));
    sprintf(szCommandLine, "\"%s\" --pid %d --sigs %s", szFilePath, GetCurrentProcessId(), BuildSignatureParameters(m_Signatures).c_str());

    // Initialize variables
    STARTUPINFOA        startupInfo = {0};
    PROCESS_INFORMATION processInfo = {0};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.hStdOutput = NULL;
    startupInfo.hStdError = NULL;
    startupInfo.hStdInput = NULL;

    // Create the process
    if (!CreateProcessA(nullptr,                  // Path to the executable
                        szCommandLine,            // Command line arguments (nullptr if none)
                        nullptr,                  // Process security attributes
                        nullptr,                  // Thread security attributes
                        FALSE,                    // Inherit handles
                        CREATE_SUSPENDED,            // Creation flags
                        nullptr,                  // Use parent's environment block
                        nullptr,                  // Use parent's starting directory
                        &startupInfo,             // Pointer to STARTUPINFO structure
                        &processInfo))
    {            // Pointer to PROCESS_INFORMATION structure
        SharedUtil::AddDebugLog("Failed to create process. Error: 0x%llx", GetLastError());
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    // Get the exit code
    DWORD exitCode = 0;
    if (GetExitCodeProcess(processInfo.hProcess, &exitCode))
    {
        SharedUtil::AddDebugLog("Process finished with exit code: 0x%llx", GetLastError());
    }
    else
    {
        SharedUtil::AddDebugLog("Failed to get exit code. Error: 0x%llx", GetLastError());
    }
}

int iCounter = 0;

void CHeuristicGuard::AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            auto sig = Signature;

            m_Signatures.insert(sig);

            iCounter++;

            // CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SearchForString, reinterpret_cast<PVOID>(&iCounter), 0, 0);
            // std::thread t(&CHeuristicGuard::SearchForString);
        }
    }
}