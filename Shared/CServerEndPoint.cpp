#include "CServerEndPoint.h"

void CServerEndPoint::MeasureLatency(long lTimeoutMs)
{
    HINTERNET hInternet = InternetOpenA("AtomicShieldAgent/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet)
    {
        m_lLatencyMs = (std::numeric_limits<long>::max)();
        return;
    }

    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &lTimeoutMs, sizeof(lTimeoutMs));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &lTimeoutMs, sizeof(lTimeoutMs));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &lTimeoutMs, sizeof(lTimeoutMs));

    auto startTime = std::chrono::high_resolution_clock::now();

    HINTERNET hUrl = InternetOpenUrlA(hInternet, m_strUrl.c_str(), NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0);

    auto endTime = std::chrono::high_resolution_clock::now();

    if (hUrl)
    {
        m_lLatencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        InternetCloseHandle(hUrl);
    }
    else
    {
        m_lLatencyMs = (std::numeric_limits<long>::max)();
    }

    InternetCloseHandle(hInternet);
}