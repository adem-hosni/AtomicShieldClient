#include "CSafeAntiCheat.h"
#include "Common.h"
#include "SharedUtil.h"
#include <condition_variable>
#include <future>

CSafeNetwork* g_pSafeNetwork = new CSafeNetwork();

CSafeNetwork::CSafeNetwork() : m_bConnected(false)
{
    m_pWebSocket = new ix::WebSocket();
}

CSafeNetwork::~CSafeNetwork()
{
}

bool CSafeNetwork::Connect()
{
    if (!ix::initNetSystem())
        return false;

    m_pWebSocket->setUrl(WEBSOCKET_BASE_URL "/c/safeguardagent/");

    m_pWebSocket->setOnMessageCallback(std::bind(&CSafeNetwork::OnReceivePacket, this, std::placeholders::_1));

    ix::WebSocketInitResult result = m_pWebSocket->connect(32);
    m_pWebSocket->start();

    if (result.success)
        m_pWebSocket->enableAutomaticReconnection();

    return result.success;
}

void CSafeNetwork::SendPacket(eEaglePacketID PacketID, jsoncons::json Data)
{
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet Type
    PacketJson["type"] = (unsigned short)PacketID;

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to eagle master server
    m_pWebSocket->send(PacketJson.to_string());
}

void CSafeNetwork::OnConnect()
{
    if (!g_pSafeAntiCheat->GetEagleNetwork()->JoinNetwork())
    {
        FreeModule(GetModuleHandle(NULL));
        return;
    }

    if (!g_pSafeAntiCheat->GetEagleNetwork()->SyncMaliciousSignatures())
        MessageBox(0, "Failed to sync malicious signatures!", "Error", 0);
}

jsoncons::json CSafeNetwork::WaitReponse(eEaglePacketID PacketID)
{
    while (m_UnhandledPackets.find(PacketID) == m_UnhandledPackets.end())
    {
        Sleep(10);
    }
    jsoncons::json Response = m_UnhandledPackets[PacketID];
    m_UnhandledPackets.erase(PacketID);
    return Response;
}

bool CSafeNetwork::JoinNetwork()
{
    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["mta_serial"] = g_pHWID->GetMTASerial();
    RequestData["username"] = g_pHWID->GetWindowsUsername();
    RequestData["disks"] = g_pHWID->GetDisksSerialNumber();
    RequestData["cpu"] = g_pHWID->GetCPUsSerials();
    RequestData["motherboard_serial"] = g_pHWID->GetMotherBoardSerial();
    RequestData["bios"] = g_pHWID->GetBIOSVersion();
    RequestData["pnp_device"] = g_pHWID->GetPNPDeviceID();
    RequestData["computer_name"] = g_pHWID->GetComputerName_();

    SendPacket(eEaglePacketID::NETWORK_JOIN, RequestData);
    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    if (!Response["success"].as_bool())
        MessageBox(0, Response["message"].as_string().c_str(), "ERROR", MB_ICONERROR);

    return Response["success"].as_bool();
}

bool CSafeNetwork::SyncMaliciousSignatures()
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
            std::vector<std::string> vSignatures;
            for (const auto& element : SignaturesList.array_range())
            {
                vSignatures.push_back(element.as<std::string>());
            }
            m_Signatures[SignatureTitle] = vSignatures;
        }
    }
    _beginthread((_beginthread_proc_type)&CSafeAntiCheat::DoPulse, NULL, g_pSafeAntiCheat);
    return true;
}

void CSafeNetwork::DoPulse()
{
    //printf("state: %d\n", m_pWebSocket->getReadyState());
}

void CSafeNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
        {
            _beginthread((_beginthread_proc_type)&CSafeNetwork::OnConnect, NULL, this);
            break;
        }

        case ix::WebSocketMessageType::Message:
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            jsoncons::json              json = jsoncons::json::parse(Message->str);
            m_UnhandledPackets.insert_or_assign((eEaglePacketID)json["type"].as<int>(), json);
            m_condition.notify_all();
            break;
        }

        case ix::WebSocketMessageType::Close:
        {
            printf("closed\n");
            MessageBox(0, "closed", 0, 0);
            MessageBox(0, Message->str.c_str(), "Network Error", 0);
            break;
        }

        case ix::WebSocketMessageType::Error:
            MessageBox(0, Message->str.c_str(), "Network Error", 0);
            break;
    }
}