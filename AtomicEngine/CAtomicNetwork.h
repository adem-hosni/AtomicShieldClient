#pragma once
#include <winhttp.h>
#include "StdInc.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <mutex>
#include <map>
#include <queue>

class CAtomicNetwork
{
public:
    CAtomicNetwork();
    ~CAtomicNetwork();

    void SetServerEndPoint(std::string& strServerEndPoint) { m_strServerEndPoint = strServerEndPoint; }

    bool           Connect();
    bool           IsConnected() { return m_bConnected; }
    bool           IsJoinedNetwork() { return m_bNetworkJoined; }
    void           SendPacket(eAtomicPacket PacketID, jsoncons::json Data = jsoncons::json(), bool bHighPriority = false);
    jsoncons::json WaitReponse(eAtomicPacket PacketID);

    static void OnConnect();
    static void StaticPulse(void* pContext);
    void        DoPulse();

    ix::WebSocket* GetWebSocket() { return m_pWebSocket; }

    std::string GetPublicIP();

    bool JoinNetwork();
    bool SyncMaliciousSignatures(jsoncons::json& Signatures);
    void HandleRequestScreenshot(jsoncons::json& Packet);
    void HandleEngineShutdown();
    void HandleUploadDebugLogs(jsoncons::json& Packet);
    void HandleFileUpload(jsoncons::json& Packet);
    void HandleRunScanners(jsoncons::json& Packet);

    void OnReceivePacket(const ix::WebSocketMessagePtr& Message);

    void HandleIncomingPacket(jsoncons::json Packet);

    void RequestFileUpload(std::string strFilePath);

    std::map<std::string, std::vector<std::string>> GetSignatures() { return m_Signatures; }

    ix::ReadyState GetReadyState() { return m_pWebSocket->getReadyState(); }
    void           Disconnect(std::string strReason);

private:
    std::string                                     m_strServerEndPoint;
    ix::WebSocket*                                  m_pWebSocket;
    std::mutex                                      m_mutex;
    std::condition_variable                         m_condition;
    std::map<eAtomicPacket, jsoncons::json>         m_PendingResponses;
    std::map<std::string, std::vector<std::string>> m_Signatures;
    std::queue<std::string>                         m_vPendingPackets;
    unsigned long long                              m_ullLastPingTime;

    bool m_bNetworkJoined;
    bool m_bConnected;
};