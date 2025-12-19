#pragma once
#include <windows.h>

class CPipeServer
{
public:
    CPipeServer(const wchar_t* pipeName);
    ~CPipeServer();

    bool Run();

private:
    bool Create();
    bool WaitForClient();
    bool HandleClient();
    bool ExecuteCode(const BYTE* code, DWORD size, char* outMsg, DWORD outMsgSize);

    bool SendSuccess(const char* message);
    bool SendFailure(const char* error);

    void DoPulse();

private:
    HANDLE         m_hPipe;
    const wchar_t* m_PipeName;
};
