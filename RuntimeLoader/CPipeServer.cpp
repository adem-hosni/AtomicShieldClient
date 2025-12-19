#include "CPipeServer.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include "RuntimeLoader.h"

#define PIPE_BUFFER_SIZE 4096
#define LOAD_PREFIX      "load_code:"
#define LOAD_PREFIX_LEN  10

CPipeServer::CPipeServer(const wchar_t* pipeName) : m_hPipe(INVALID_HANDLE_VALUE), m_PipeName(pipeName)
{
}

CPipeServer::~CPipeServer()
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
        CloseHandle(m_hPipe);
}

bool CPipeServer::Create()
{
    m_hPipe = CreateNamedPipeW(m_PipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0,
                               nullptr);

    return (m_hPipe != INVALID_HANDLE_VALUE);
}

bool CPipeServer::WaitForClient()
{
    BOOL connected = ConnectNamedPipe(m_hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    return connected == TRUE;
}

bool CPipeServer::Run()
{
    if (!Create())
        return false;

    std::thread t(&CPipeServer::DoPulse, this);
}

void CPipeServer::DoPulse()
{
    while (true)
    {
        if (!WaitForClient())
        {
            Sleep(200);
            continue;
        }

        HandleClient();
        
        FlushFileBuffers(m_hPipe);

        Sleep(500);
    }
}

bool CPipeServer::HandleClient()
{
    BYTE  buffer[PIPE_BUFFER_SIZE] = {};
    DWORD bytesRead = 0;

    if (!ReadFile(m_hPipe, buffer, sizeof(buffer), &bytesRead, nullptr))
        return SendFailure("read_failed");

    if (bytesRead <= LOAD_PREFIX_LEN)
        return SendFailure("buffer_too_small");

    if (memcmp(buffer, LOAD_PREFIX, LOAD_PREFIX_LEN) != 0)
        return SendFailure("invalid_command");

    const BYTE* codePtr = buffer + LOAD_PREFIX_LEN;
    DWORD       codeSize = bytesRead - LOAD_PREFIX_LEN;

    char execMsg[256] = {};

    if (!ExecuteCode(codePtr, codeSize, execMsg, sizeof(execMsg)))
        return SendFailure(execMsg);

    return SendSuccess(execMsg);
}

bool CPipeServer::ExecuteCode(const BYTE* code, DWORD size, char* outMsg, DWORD outMsgSize)
{
    if (!code || size == 0)
    {
        strncpy_s(outMsg, outMsgSize, "empty_code", _TRUNCATE);
        return false;
    }

    int iret = RuntimeLoader::SelfMapModule((BYTE*)code, size);

    snprintf(outMsg, outMsgSize, "executed_%lu_bytes-%x", size, iret);
    return iret == 0;
}

bool CPipeServer::SendSuccess(const char* message)
{
    char  buffer[512] = {};
    DWORD written = 0;

    snprintf(buffer, sizeof(buffer), "load_success:%s", message);

    return WriteFile(m_hPipe, buffer, (DWORD)strlen(buffer) + 1, &written, nullptr) == TRUE;
}

bool CPipeServer::SendFailure(const char* error)
{
    char  buffer[512] = {};
    DWORD written = 0;

    snprintf(buffer, sizeof(buffer), "load_failure:%s", error);

    return WriteFile(m_hPipe, buffer, (DWORD)strlen(buffer) + 1, &written, nullptr) == TRUE;
}
