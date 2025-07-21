#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <chrono>
#include <Windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

class CServerEndPoint
{
public:
    CServerEndPoint(std::string url) : m_strUrl(url), m_lLatencyMs(0) {}

    const std::string& GetUrl() const { return m_strUrl; }
    long               GetLatencyMs() const { return m_lLatencyMs; }
    void               MeasureLatency(long lTimeoutMs = 1500);

private:
    static size_t DiscardCallback(void* contents, size_t size, size_t nmemb, void* userp) { return size * nmemb; }

    std::string m_strUrl;
    long        m_lLatencyMs;
};
