#pragma once
#include <winhttp.h>
#include "StdInc.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <mutex>
#include <map>
#include <queue>

enum class eHeartbeatType
{
    HEURISTIC_GUARD = 1,
    PROCESS_GUARD = 2,
};

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
    void        Ping(eHeartbeatType HeartbeatType);

    ix::WebSocket* GetWebSocket() { return m_pWebSocket; }

    std::string GetIPAddressChain();

    bool JoinNetwork();
    
    bool SyncMaliciousSignatures(jsoncons::json& Signatures);
    void RequestFileUpload(std::string strFilePath, std::string strFileHash);

    void HandleRequestScreenshot(jsoncons::json& Packet);
    void HandleEngineShutdown();
    void HandleUploadDebugLogs(jsoncons::json& Packet);
    void HandleFileUpload(jsoncons::json& Packet);
    void HandleRunScanners(jsoncons::json& Packet);
    void HandleIncomingPacket(jsoncons::json Packet);
    void HandleReloadEngine(jsoncons::json& Packet);

    void OnReceivePacket(const ix::WebSocketMessagePtr& Message);

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