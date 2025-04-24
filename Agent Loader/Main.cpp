#include "StdInc.h"

int main(int)
{
    RuntimeImportResolver::ResolveCurrentImports();

    FILE* file = fopen("C:\\AtomicShield\\AtomicShieldClient\\Build\\Atomic Agent.Release.dll", "rb");
    if (!file)
    {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = new char[fileSize];
    fread(buffer, 1, fileSize, file);
    fclose(file);

    ManualLoadBuffer((BYTE*)buffer, fileSize);

    return 0;
}