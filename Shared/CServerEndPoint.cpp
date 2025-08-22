#define WIN32_LEAN_AND_MEAN
#include "CServerEndPoint.h"
#include "SharedUtil.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#pragma comment(lib, "Ws2_32.lib")


void CServerEndPoint::MeasureLatency(long lTimeoutMs)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        SharedUtil::AddDebugLog("WSAStartup failed: %d", WSAGetLastError());
        m_lLatencyMs = -1;
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        SharedUtil::AddDebugLog("socket() failed: %d", WSAGetLastError());
        WSACleanup();
        m_lLatencyMs = -1;
        return;
    }

    // Parse IP and port (default 80)
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
    if (inet_pton(AF_INET, m_strUrl.c_str(), &serverAddr.sin_addr) <= 0)
    {
        SharedUtil::AddDebugLog("inet_pton failed for %s", m_strUrl.c_str());
        closesocket(sock);
        WSACleanup();
        m_lLatencyMs = -1;
        return;
    }

    // Set non-blocking mode
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    SharedUtil::AddDebugLog("Attempting connection to %s:%d", m_strUrl.c_str(), 443);

    auto start = std::chrono::high_resolution_clock::now();
    int  result = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval tv;
        tv.tv_sec = lTimeoutMs / 1000;
        tv.tv_usec = (lTimeoutMs % 1000) * 1000;

        int sel = select(0, nullptr, &writeSet, nullptr, &tv);
        if (sel > 0)
        {
            int optVal;
            int optLen = sizeof(optVal);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optVal, &optLen);
            if (optVal == 0)
            {
                auto end = std::chrono::high_resolution_clock::now();
                m_lLatencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                SharedUtil::AddDebugLog("Connection successful to %s in %ld ms", m_strUrl.c_str(), m_lLatencyMs);
            }
            else
            {
                SharedUtil::AddDebugLog("Connection failed to %s: %d", m_strUrl.c_str(), optVal);
                m_lLatencyMs = -1;
            }
        }
        else if (sel == 0)
        {
            SharedUtil::AddDebugLog("Connection to %s timed out after %ld ms", m_strUrl.c_str(), lTimeoutMs);
            m_lLatencyMs = -1;
        }
        else
        {
            SharedUtil::AddDebugLog("select() error for %s: %d", m_strUrl.c_str(), WSAGetLastError());
            m_lLatencyMs = -1;
        }
    }
    else if (result == 0)
    {
        auto end = std::chrono::high_resolution_clock::now();
        m_lLatencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        SharedUtil::AddDebugLog("Immediate connection to %s in %ld ms", m_strUrl.c_str(), m_lLatencyMs);
    }
    else
    {
        SharedUtil::AddDebugLog("connect() error for %s: %d", m_strUrl.c_str(), WSAGetLastError());
        m_lLatencyMs = -1;
    }

    closesocket(sock);
    WSACleanup();
}
