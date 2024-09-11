#pragma once
#include <Windows.h>

namespace SharedChecks
{
    void MaliciousProcessAlert(char* szProcessName);
    void CheckProcessList(void (*found_process)(char* szProcessName));
}