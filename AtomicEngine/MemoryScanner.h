#pragma once
#include "StdInc.h"

namespace MemoryScanner
{
    std::vector<std::pair<uint8_t, bool>> ParseSignature(const std::string& pattern);
    uint8_t*                              FindPattern(uint8_t* buffer, size_t size, const std::string& pattern);
    uint8_t*                              FindPattern(uint8_t* buffer, size_t size, const char* pattern, const char* mask);
}            // namespace MemoryScanner