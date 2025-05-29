#pragma once
#include "StdInc.h"

class CCrashHandler
{
public:
    static void Initialize();
    static LONG __stdcall SEHTranslator(EXCEPTION_POINTERS* pExp);
    static bool           UploadCrashReport(EXCEPTION_POINTERS* pException);
    static jsoncons::json GenerateCrashReport(EXCEPTION_POINTERS* pException);
};
