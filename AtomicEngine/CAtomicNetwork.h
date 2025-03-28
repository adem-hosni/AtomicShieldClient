#pragma once
#include "StdInc.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <mutex>
#include <map>

class CAtomicNetwork
{
public:
    CAtomicNetwork();
    ~CAtomicNetwork();

    bool           Connect();
    bool           IsConnected() { return m_bConnected; }
    bool           IsJoinedNetwork() { return m_bNetworkJoined; }
    void           SendPacket(eAtomicPacket PacketID, jsoncons::json Data = jsoncons::json());
    jsoncons::json WaitReponse(eAtomicPacket PacketID);

    static void OnConnect();
    static void StaticPulse(void* pContext);
    void        DoPulse();

    ix::WebSocket* GetWebSocket() { return m_pWebSocket; }

    bool JoinNetwork();
    bool SyncMaliciousSignatures(jsoncons::json& Signatures);
    void HandleRequestScreenshot();
    void HandleRunScanners(jsoncons::json& Packet);

    void OnReceivePacket(const ix::WebSocketMessagePtr& Message);

    void HandleIncomingPacket(jsoncons::json Packet);

    std::map<std::string, std::vector<std::string>> GetSignatures() { return m_Signatures; }

    ix::ReadyState GetReadyState() { return m_pWebSocket->getReadyState(); }
    void           Disconnect(std::string strReason);

private:
    void Reconnect();

    ix::WebSocket*                                  m_pWebSocket;
    std::mutex                                      m_mutex;
    std::condition_variable                         m_condition;
    std::map<eAtomicPacket, jsoncons::json>         m_UnhandledPackets;
    std::map<std::string, std::vector<std::string>> m_Signatures;

    bool m_bNetworkJoined;
    bool m_bConnected;
};