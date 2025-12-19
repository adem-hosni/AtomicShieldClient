#include <iostream>
#include <windows.h>
#include "RuntimeLoader.h"
#include "CPipeServer.h"

int main()
{
    FILE* pFile = fopen("C:\\AtomicShield\\AtomicShieldClient\\Build\\Atomic Engine.dll", "rb");
    fseek(pFile, 0, SEEK_END);
    SIZE_T fsize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    BYTE* buffer = new BYTE[fsize];
    fread(buffer, 1, fsize, pFile);
    fclose(pFile);

    CPipeServer* pPipeServer = new CPipeServer(L"\\\\.\\pipe\\AtomicRuntime");
    pPipeServer->Run();
}
