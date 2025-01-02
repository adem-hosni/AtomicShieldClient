#pragma once
#include <ctime>

#define GAME_MEMORY_SCAN_INTERVAL            5

struct STiming
{
    long long llLastMemoryScan = time(NULL);
};