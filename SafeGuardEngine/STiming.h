#pragma once
#include <ctime>

#define GAME_ANTICHEAT_STATUS_CHECK_INTERVAL 5
#define GAME_MEMORY_SCAN_INTERVAL            5

struct STiming
{
    long long llLastGameAntiCheatCheck = time(NULL);
    long long llLastMemoryScan = time(NULL);
};