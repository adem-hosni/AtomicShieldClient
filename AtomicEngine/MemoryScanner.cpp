#include "StdInc.h"

std::vector<std::pair<uint8_t, bool>> MemoryScanner::ParseSignature(const std::string& pattern)
{
    std::vector<std::pair<uint8_t, bool>> signature;
    std::istringstream                    stream(pattern);
    std::string                           byteStr;

    while (stream >> byteStr)
    {
        if (byteStr == "??" || byteStr == "?")
        {
            signature.emplace_back(0x00, true);            // wildcard
        }
        else
        {
            signature.emplace_back(static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16)), false);
        }
    }

    return signature;
}

uint8_t* MemoryScanner::FindPattern(uint8_t* buffer, size_t size, const std::string& pattern)
{
    auto   signature = ParseSignature(pattern);
    size_t patternLength = signature.size();

    for (size_t i = 0; i <= size - patternLength; ++i)
    {
        bool matched = true;
        for (size_t j = 0; j < patternLength; ++j)
        {
            if (!signature[j].second && buffer[i + j] != signature[j].first)
            {
                matched = false;
                break;
            }
        }
        if (matched)
        {
            return buffer + i;
        }
    }

    return nullptr;
}


uint8_t* MemoryScanner::FindPattern(uint8_t* buffer, size_t size, const char* pattern, const char* mask)
{
    size_t patternLength = std::strlen(mask);

    for (size_t i = 0; i <= size - patternLength; ++i)
    {
        bool matched = true;
        for (size_t j = 0; j < patternLength; ++j)
        {
            if (mask[j] == 'x' && buffer[i + j] != static_cast<uint8_t>(pattern[j]))
            {
                matched = false;
                break;
            }
        }
        if (matched)
        {
            return buffer + i;
        }
    }

    return nullptr;
}