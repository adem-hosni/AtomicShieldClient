#include "CAtomicAntiCheat.h"
#include "Common.h"
#include "SharedUtil.h"
#include <condition_variable>
#include <future>

CAtomicNetwork::CAtomicNetwork() : m_bConnected(false)
{
    m_pWebSocket = new ix::WebSocket();
    m_bNetworkJoined = false;
}

CAtomicNetwork::~CAtomicNetwork()
{
}

bool CAtomicNetwork::Connect()
{
    if (!ix::initNetSystem())
        return false;

    m_pWebSocket->setUrl(WEBSOCKET_BASE_URL "/c/atomicshieldagent/");

    m_pWebSocket->setOnMessageCallback(std::bind(&CAtomicNetwork::OnReceivePacket, this, std::placeholders::_1));

    m_pWebSocket->setPingInterval(3);

    ix::WebSocketInitResult result = m_pWebSocket->connect(32);
    m_pWebSocket->start();

    if (result.success)
    {
        m_pWebSocket->enableAutomaticReconnection();

        while (!m_bNetworkJoined)
            Sleep(25);
    }

    return result.success;
}

void CAtomicNetwork::SendPacket(eAtomicPacket PacketID, jsoncons::json Data)
{
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet type and the unix timestamp
    PacketJson["type"] = (unsigned short)PacketID;
    PacketJson["ut"] = time(NULL);

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to eagle master server
    m_pWebSocket->send(PacketJson.to_string());
}

void CAtomicNetwork::OnConnect()
{
    if (!g_pAtomicAntiCheat->GetNetwork()->JoinNetwork())
    {
        FreeModule(GetModuleHandle(NULL));
        return;
    }

    if (!g_pAtomicAntiCheat->GetNetwork()->SyncMaliciousSignatures())
        MessageBox(0, "Failed to sync malicious signatures!", "Error", 0);
}

void CAtomicNetwork::StaticPulse(void* pContext)
{
    CAtomicNetwork* pNetwork = reinterpret_cast<CAtomicNetwork*>(pContext);
    pNetwork->DoPulse();
}

jsoncons::json CAtomicNetwork::WaitReponse(eAtomicPacket PacketID)
{
    while (m_UnhandledPackets.find(PacketID) == m_UnhandledPackets.end())
    {
        Sleep(10);
    }
    jsoncons::json Response = m_UnhandledPackets[PacketID];

    // Check if the unix timestamp received is tampered
    if (time(NULL) - Response["ut"].as<DWORD>() >= 20)
    {
        __fastfail(0);
        // Return an empty data to crash the engine if the __fastfail was tampered
        jsoncons::json j;
        return j;
    }

    m_UnhandledPackets.erase(PacketID);
    return Response;
}

bool CAtomicNetwork::JoinNetwork()
{
    jsoncons::json RequestHWID;
    RequestHWID["extra"] = g_pHWID->GetExtraData();
    RequestHWID["username"] = g_pHWID->GetWindowsUsername();
    RequestHWID["disks"] = g_pHWID->GetDisksSerialNumber();
    RequestHWID["cpu"] = g_pHWID->GetCPUsSerials();
    RequestHWID["motherboard_serial"] = g_pHWID->GetMotherBoardSerial();
    RequestHWID["bios"] = g_pHWID->GetBIOSVersion();
    RequestHWID["pnp_device"] = g_pHWID->GetPNPDeviceID();
    RequestHWID["computer_name"] = g_pHWID->GetComputerName_();
    RequestHWID["monitor"] = g_pHWID->GetMonitorSerial();
    
    jsoncons::json RequestData;
    RequestData["hwid"] = RequestHWID;
    RequestData["cache"] = g_pAtomicAntiCheat->GetCurrentHWIDCache();
    RequestData["engine_type"] = 2; // FiveM

    SendPacket(eAtomicPacket::NETWORK_JOIN, RequestData);
    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    if (Response["success"].as_bool())
    {
        g_pHWID->StoreHWIDCaches(RequestHWID);
        g_pAtomicAntiCheat->StartPulse();
    }
    else
    {
        MessageBox(0, Response["message"].as_string().c_str(), "ERROR", MB_ICONERROR);
    }

    m_bNetworkJoined = Response["success"].as_bool();
    return Response["success"].as_bool();
}

bool CAtomicNetwork::SyncMaliciousSignatures()
{
    SendPacket(SYNC_SIGNATURES);
    jsoncons::json Response = WaitReponse(SYNC_SIGNATURES);
    jsoncons::json Signatures = Response["signatures"];

    for (const auto& Item : Signatures.object_range())
    {
        const std::string&    SignatureTitle = Item.key();
        const jsoncons::json& SignaturesList = Item.value();

        if (SignaturesList.is_array())
        {
            std::vector<std::wstring> vSignatures = {};
            for (const auto& element : SignaturesList.array_range())
            {
                vSignatures.push_back(element.as<std::wstring>());
            }
            m_Signatures[SignatureTitle] = vSignatures;
            vSignatures.clear();
        }
    }
    g_pAtomicAntiCheat->GetGuardManager()->GetHeuristicGuard()->AddSignatures(m_Signatures);
    
    Signatures.clear();
    m_Signatures.clear();


    return true;
}

void CAtomicNetwork::DoPulse()
{
    //printf("state: %d\n", m_pWebSocket->getReadyState());
}

void CAtomicNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
        {
            _beginthread((_beginthread_proc_type)&CAtomicNetwork::OnConnect, NULL, this);
            break;
        }

        case ix::WebSocketMessageType::Message:
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            jsoncons::json              json = jsoncons::json::parse(Message->str);
            m_UnhandledPackets.insert_or_assign((eAtomicPacket)json["type"].as<int>(), json);
            m_condition.notify_all();
            break;
        }

        case ix::WebSocketMessageType::Close:
        {
            Reconnect();
            break;
        }

        case ix::WebSocketMessageType::Error:
            Reconnect();
            break;
    }
}

void CAtomicNetwork::Reconnect()
{
    m_bNetworkJoined = false;

    while (m_pWebSocket->getReadyState() == ix::ReadyState::Closed || m_pWebSocket->getReadyState() == ix::ReadyState::Closing)
    {
        Connect();
        Sleep(2 * 1000);
    }
}