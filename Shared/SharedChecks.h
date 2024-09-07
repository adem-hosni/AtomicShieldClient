#pragma once
#include <Windows.h>

namespace SharedChecks
{
    void CheckProcessList(void(*found_process)(char* szProcessName));
}