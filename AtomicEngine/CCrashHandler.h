#pragma once
#include "StdInc.h"

class CCrashHandler
{
public:
    static void Initialize();
    static LONG __stdcall SEHTranslator(EXCEPTION_POINTERS* pExp);
    static bool           UploadCrashReport(jsoncons::json& CrashReport);
    static jsoncons::json GenerateCrashReport(EXCEPTION_POINTERS* pException, DWORD64 dwModuleBase);
};
