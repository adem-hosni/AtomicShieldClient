#include "CAtomicAntiCheat.h"
#include "CAtomicCore.h"
#include "Common.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "SharedUtil.h"
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <future>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

CAtomicNetwork::CAtomicNetwork() : m_bConnected(false), m_bNetworkJoined(false), m_ullLastPingTime(NULL)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::CAtomicNetwork - ctor entered");
    m_pWebSocket = new ix::WebSocket();
    SharedUtil::AddDebugLog("CAtomicNetwork::CAtomicNetwork - WebSocket created");
}

CAtomicNetwork::~CAtomicNetwork()
{
    SharedUtil::AddDebugLog("CAtomicNetwork::~CAtomicNetwork - dtor entered");
}

bool CAtomicNetwork::Connect()
{
    SharedUtil::AddDebugLog("CAtomicNetwork::Connect - entered");
    delete m_pWebSocket;
    m_pWebSocket = new ix::WebSocket();
    SharedUtil::AddDebugLog("CAtomicNetwork::Connect - WebSocket reset");

    m_bNetworkJoined = false;

    SharedUtil::AddDebugLog("Initializing network system...");
    ix::initNetSystem();

    m_pWebSocket->setUrl(m_strServerEndPoint + "/c/atomicshieldagent/");
    SharedUtil::AddDebugLog("CAtomicNetwork::Connect - setUrl to %s", m_strServerEndPoint.c_str());

    SharedUtil::AddDebugLog("Setting up websocket callbacks...");
    m_pWebSocket->setOnMessageCallback(std::bind(&CAtomicNetwork::OnReceivePacket, this, std::placeholders::_1));
    m_pWebSocket->setPingInterval(10);
    m_pWebSocket->enablePerMessageDeflate();
    m_pWebSocket->enablePong();
    m_pWebSocket->disableAutomaticReconnection();

    SharedUtil::AddDebugLog("Performing connection to %s", m_strServerEndPoint.c_str());
    ix::WebSocketInitResult result = m_pWebSocket->connect(32);

    SharedUtil::AddDebugLog("Starting websocket thread...");
    m_pWebSocket->start();

    if (result.success)
    {
        while (!m_bNetworkJoined)
        {
            SharedUtil::AddDebugLog("Waiting to join the AtomicShield Network...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        SharedUtil::AddDebugLog("Successfully joined the AtomicShield Network!");
    }
    else
    {
        SharedUtil::AddDebugLog("Couldn't connect to the websocket due to %s", result.errorStr.c_str());
    }

    SharedUtil::AddDebugLog("CAtomicNetwork::Connect - returning %d", result.success);
    return result.success;
}

void CAtomicNetwork::SendPacket(eAtomicPacket PacketID, jsoncons::json Data, bool bHighPriority)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::SendPacket - entered PacketID=%d HighPriority=%d", (int)PacketID, bHighPriority);
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet type and the unix timestamp
    PacketJson["type"] = (unsigned short)PacketID;
    PacketJson["ut"] = time(NULL);

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to master server
    std::string buffer = g_pAtomicCore->Encrypt(PacketJson.to_string());
    SharedUtil::AddDebugLog("CAtomicNetwork::SendPacket - encrypted buffer length=%zu", buffer.size());

    if (bHighPriority)
    {
        m_pWebSocket->send(SharedUtil::Base64Encode(buffer));
        SharedUtil::AddDebugLog("CAtomicNetwork::SendPacket - sent high priority packet");
        return;
    }
    m_vPendingPackets.push(SharedUtil::Base64Encode(buffer));
    SharedUtil::AddDebugLog("CAtomicNetwork::SendPacket - queued packet");
}

void CAtomicNetwork::OnConnect()
{
    SharedUtil::AddDebugLog("CAtomicNetwork::OnConnect - entered");
    SharedUtil::AddDebugLog("Connected to the AtomicShield Server!");
    if (!g_pAtomicAntiCheat->GetNetwork()->JoinNetwork())
    {
        SharedUtil::AddDebugLog("Failed to join the AtomicShield Network, shutting down the engine...");
        FreeModule(GetModuleHandle(NULL));
        return;
    }
    SharedUtil::AddDebugLog("CAtomicNetwork::OnConnect - joined network");
}

void CAtomicNetwork::StaticPulse(void* pContext)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::StaticPulse - entered");
    CAtomicNetwork* pNetwork = reinterpret_cast<CAtomicNetwork*>(pContext);
    pNetwork->DoPulse();
    SharedUtil::AddDebugLog("CAtomicNetwork::StaticPulse - pulse done");
}

jsoncons::json CAtomicNetwork::WaitReponse(eAtomicPacket PacketID)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::WaitReponse - entered PacketID=%d", (int)PacketID);
    while (m_PendingResponses.find(PacketID) == m_PendingResponses.end())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    jsoncons::json Response = m_PendingResponses[PacketID];
    SharedUtil::AddDebugLog("CAtomicNetwork::WaitReponse - got response");

    m_PendingResponses.erase(PacketID);
    SharedUtil::AddDebugLog("CAtomicNetwork::WaitReponse - erased response");
    return Response;
}

std::string CAtomicNetwork::GetIPAddressChain()
{
    SharedUtil::AddDebugLog("CAtomicNetwork::GetIPAddressChain - entered");
    struct IPApi
    {
        std::wstring host;
        std::wstring path;
        bool         json;            // if true, extract "ip" from {"ip": "..."]
    };

    std::vector<IPApi> apis = {
        {L"api.ipify.org", L"/?format=json", true}, {L"api.myip.com", L"/", true},  {L"ifconfig.me", L"/ip", false},
        {L"checkip.amazonaws.com", L"/", false},    {L"ipinfo.io", L"/json", true}, {L"ipwho.is", L"/", true},
    };

    std::vector<std::string> collectedIPs;

    for (const auto& api : apis)
    {
        HINTERNET hSession = WinHttpOpen(L"AtomicShield/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
        if (!hSession)
        {
            SharedUtil::AddDebugLog("[HTTP] WinHttpOpen failed: %lu", GetLastError());
            continue;
        }

        HINTERNET hConnect = WinHttpConnect(hSession, api.host.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!hConnect)
        {
            SharedUtil::AddDebugLog("[HTTP] WinHttpConnect failed: %lu", GetLastError());
            WinHttpCloseHandle(hSession);
            continue;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", api.path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest)
        {
            SharedUtil::AddDebugLog("[HTTP] WinHttpOpenRequest failed: %lu", GetLastError());
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        {
            SharedUtil::AddDebugLog("[HTTP] WinHttpSendRequest failed: %lu", GetLastError());
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr))
        {
            SharedUtil::AddDebugLog("[HTTP] WinHttpReceiveResponse failed: %lu", GetLastError());
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        std::string response;
        DWORD       dwSize = 0;
        do
        {
            DWORD dwDownloaded = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0)
                break;

            std::string buffer(dwSize, 0);
            if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
                break;

            response.append(buffer.c_str(), dwDownloaded);
        } while (dwSize > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (response.empty())
        {
            SharedUtil::AddDebugLog("CAtomicNetwork::GetIPAddressChain - empty response from %ws", api.host.c_str());
            continue;
        }

        std::string extractedIp;
        if (api.json)
        {
            size_t ipKey = response.find("\"ip\"");
            if (ipKey != std::string::npos)
            {
                size_t quote1 = response.find('"', ipKey + 4);
                size_t quote2 = response.find('"', quote1 + 1);
                if (quote1 != std::string::npos && quote2 != std::string::npos)
                    extractedIp = response.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }
        else
        {
            // raw IP, strip whitespace
            size_t start = response.find_first_not_of(" \r\n\t");
            size_t end = response.find_last_not_of(" \r\n\t");
            if (start != std::string::npos && end != std::string::npos)
                extractedIp = response.substr(start, end - start + 1);
        }

        if (!extractedIp.empty())
        {
            SharedUtil::AddDebugLog("CAtomicNetwork::GetIPAddressChain - extracted IP %s", extractedIp.c_str());
            collectedIPs.push_back(extractedIp);
        }
        else
        {
            SharedUtil::AddDebugLog("CAtomicNetwork::GetIPAddressChain - failed to parse IP from %ws response: %s", api.host.c_str(), response.c_str());
        }
    }

    if (collectedIPs.empty())
    {
        SharedUtil::AddDebugLog("[AtomicShield] All IP API requests failed.");
        return "";
    }

    std::string combined;
    for (size_t i = 0; i < collectedIPs.size(); ++i)
    {
        if (combined.find(collectedIPs[i]) != std::string::npos)
            continue;

        if (i != 0)
            combined += "-";
        combined += collectedIPs[i];
    }
    SharedUtil::AddDebugLog("CAtomicNetwork::GetIPAddressChain - returning '%s'", combined.c_str());
    return combined;
}


bool CAtomicNetwork::JoinNetwork()
{
    SharedUtil::AddDebugLog("CAtomicNetwork::JoinNetwork - entered");
    jsoncons::json RequestData;
    RequestData["ip"] = GetIPAddressChain();
    SharedUtil::AddDebugLog("Connecting With %s", RequestData["ip"].as_string().c_str());

    SharedUtil::AddDebugLog("Joining Network...");

    jsoncons::json RequestHWID;
    RequestHWID["username"] = g_pHWID->GetWindowsUsername();
    RequestHWID["disks"] = g_pHWID->GetDisksSerialNumber();
    RequestHWID["cpu"] = g_pHWID->GetCPUsSerials();
    RequestHWID["motherboard_serial"] = g_pHWID->GetMotherBoardSerial();
    RequestHWID["bios"] = g_pHWID->GetBIOSVersion();
    RequestHWID["pnp_device"] = g_pHWID->GetPNPDeviceID();
    RequestHWID["computer_name"] = g_pHWID->GetComputerName_();
    RequestHWID["monitor"] = g_pHWID->GetMonitorSerial();
    RequestHWID["steam"] = g_pHWID->GetSteamID();

    SharedUtil::AddDebugLog("Encoding HWID data...");

    for (auto& kv : RequestHWID.object_range())
    {
        const std::string& key = kv.key();
        jsoncons::json&    value = RequestHWID[key];

        if (value.is_array())
        {
            jsoncons::json new_array = jsoncons::json::array();
            for (const auto& item : value.array_range())
            {
                if (item.is_string())
                    new_array.push_back(SharedUtil::Base64Encode(item.as<std::string>()));
                else
                    new_array.push_back(SharedUtil::Base64Encode(item.to_string()));
            }
            value = std::move(new_array);
        }
        else
        {
            value = SharedUtil::Base64Encode(value.is_string() ? value.as<std::string>() : value.to_string());
        }
    }
    RequestHWID["extra"] = g_pHWID->GetExtraData();

    RequestData["hwid"] = RequestHWID;
    RequestData["cache"] = g_pAtomicAntiCheat->GetCurrentHWIDCache();
    RequestData["engine_type"] = 2;                                     // FiveM
    RequestData["build_timestamp"] = CLIENT_BUILD_TIMESTAMP;            // Some players uses an old engine

    SendPacket(eAtomicPacket::NETWORK_JOIN, RequestData, true);

    SharedUtil::AddDebugLog("Waiting for network join response...");
    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    m_bNetworkJoined = Response["success"].as_bool();
    SharedUtil::AddDebugLog("CAtomicNetwork::JoinNetwork - network join response success=%d", m_bNetworkJoined);

    if (m_bNetworkJoined)
    {
        g_pHWID->StoreHWIDCaches(RequestHWID);

        if (!g_pAtomicAntiCheat->GetNetwork()->SyncMaliciousSignatures(Response["signatures"]))
            MessageBox(0, "Failed to sync malicious signatures!", "Error", 0);
    }
    else
    {
        MessageBox(0, Response["message"].as_string().c_str(), "ERROR", MB_ICONERROR);
    }

    SharedUtil::AddDebugLog("Network Join Result -> %d", Response["success"].as_bool());
    return m_bNetworkJoined;
}

bool CAtomicNetwork::SyncMaliciousSignatures(jsoncons::json& Signatures)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::SyncMaliciousSignatures - entered");
    for (const auto& Item : Signatures.object_range())
    {
        const std::string&    SignatureTitle = Item.key();
        const jsoncons::json& SignaturesList = Item.value();

        if (SignaturesList.is_array())
        {
            std::vector<std::string> vSignatures = {};
            for (const auto& element : SignaturesList.array_range())
            {
                vSignatures.push_back(element.as<std::string>());
            }
            m_Signatures[SignatureTitle] = vSignatures;
            vSignatures.clear();
        }
    }
    g_pAtomicAntiCheat->GetGuardManager()->GetHeuristicGuard()->AddSignatures(m_Signatures);

    Signatures.clear();
    m_Signatures.clear();

    SharedUtil::AddDebugLog("CAtomicNetwork::SyncMaliciousSignatures - completed");
    return true;
}

void CAtomicNetwork::HandleRequestScreenshot(jsoncons::json& Packet)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleRequestScreenshot - entered");
    jsoncons::json response = jsoncons::json::object();
    response["request_id"] = Packet.contains("request_id") ? Packet["request_id"].as_string() : SharedUtil::GenerateRandomString(8);

    char szError[256];
    memset(szError, 0, sizeof(szError));

    std::string strScreenshotBuffer;
    bool        bSuccess = Screenshot::CreateScreenshot(&strScreenshotBuffer, szError);

    response["success"] = bSuccess;
    response["message"] = (const char*)szError;
    response["buffer"] = SharedUtil::Base64Encode(strScreenshotBuffer);

    SendPacket(REQUEST_SCREENSHOT, response);
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleRequestScreenshot - sent screenshot response");
}

void CAtomicNetwork::HandleEngineShutdown()
{
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleEngineShutdown - entered");
    if (m_bNetworkJoined)
    {
        g_pAtomicAntiCheat->Shutdown();
        SharedUtil::AddDebugLog("CAtomicNetwork::HandleEngineShutdown - shutdown called");
    }
    else
    {
        SharedUtil::AddDebugLog("Engine shutdown requested but network not joined yet.");
    }
}

void CAtomicNetwork::HandleUploadDebugLogs(jsoncons::json& Packet)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleUploadDebugLogs - entered");
    jsoncons::json response = jsoncons::json::object();
    response["request_id"] = Packet.contains("request_id") ? Packet["request_id"].as_string() : SharedUtil::GenerateRandomString(8);

    std::string strLogs;

    response["success"] = SharedUtil::GetDebugLogs(strLogs);
    response["logs"] = strLogs;

    SendPacket(REQUEST_DEBUG_LOGS, response);
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleUploadDebugLogs - sent debug logs response");
}

void CAtomicNetwork::HandleFileUpload(jsoncons::json& Packet)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleFileUpload - entered");
    jsoncons::json response = jsoncons::json::object();

    response["request_id"] = Packet.contains("request_id") ? Packet["request_id"].as_string() : SharedUtil::GenerateRandomString(8);

    std::string strFilePath = Packet["file_path"].as_string();
    if (strFilePath.empty())
    {
        response["success"] = false;
        response["message"] = "File path is empty!";
        SharedUtil::AddDebugLog("CAtomicNetwork::HandleFileUpload - file path empty");
    }
    else
    {
        std::filesystem::path filePath(strFilePath);
        if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath))
        {
            std::string   strFileBuffer;
            std::ifstream file(filePath, std::ios::in | std::ios::binary);

            if (file)
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string strFileContent = buffer.str();

                std::string strEncodedFile = SharedUtil::Base64Encode(strFileContent);
                response["success"] = true;
                response["buffer"] = strEncodedFile;
                SharedUtil::AddDebugLog("CAtomicNetwork::HandleFileUpload - file uploaded successfully");
            }
            else
            {
                response["success"] = false;
                response["message"] = "Failed to open file!";
                SharedUtil::AddDebugLog("CAtomicNetwork::HandleFileUpload - failed to open file");
            }
        }
        else
        {
            response["success"] = false;
            response["message"] = "File not found!";
            SharedUtil::AddDebugLog("CAtomicNetwork::HandleFileUpload - file not found");
        }
    }
    SendPacket(REQUEST_FILE_UPLOAD, response);
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleFileUpload - sent file upload response");
}

void CAtomicNetwork::HandleRunScanners(jsoncons::json& Packet)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleRunScanners - entered");
}

void CAtomicNetwork::Ping(eHeartbeatType HeartbeatType)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::Ping - entered HeartbeatType=%d", (int)HeartbeatType);
    if (m_pWebSocket->getReadyState() == ix::ReadyState::Open)
    {
        jsoncons::json body = jsoncons::json::object();
        body["heartbeat_type"] = (unsigned short)HeartbeatType;

        SendPacket(HEARTBEAT, body);
        m_ullLastPingTime = time(NULL);
        SharedUtil::AddDebugLog("CAtomicNetwork::Ping - sent heartbeat");
    }
    else
    {
        SharedUtil::AddDebugLog("CAtomicNetwork::Ping - websocket not open");
    }
}

void CAtomicNetwork::DoPulse()
{
    if (!m_vPendingPackets.empty())
    {
        if (m_pWebSocket->getReadyState() == ix::ReadyState::Open)
        {
            std::string strPacketBuffer = m_vPendingPackets.front();
            m_pWebSocket->send(strPacketBuffer.c_str(), false,
                               [&](int current, int total)
                               {
                                   std::this_thread::sleep_for(std::chrono::milliseconds(5));
                                   return true;
                               });
            m_vPendingPackets.pop();
            SharedUtil::AddDebugLog("CAtomicNetwork::DoPulse - sent packet");
        }
    }

    if (time(NULL) - m_ullLastPingTime > 5)
    {
        m_pWebSocket->ping("Ping");
        m_ullLastPingTime = time(NULL);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void CAtomicNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - entered type=%d", (int)Message->type);
    SharedUtil::AddDebugLog("Websocket Message callback triggered with type %d", (int)Message->type);

    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
        {
            /*std::thread t(&CAtomicNetwork::OnConnect);
            t.detach();*/
            SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - Open");
            CAtomicThread::Create(&CAtomicNetwork::OnConnect, this);
            break;
        }

        case ix::WebSocketMessageType::Message:
        {
            SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - Message");
            if (Message->type == ix::WebSocketMessageType::Message)
            {
                std::string    message_buffer = Message->str;
                std::string    decrypted_buffer = g_pAtomicCore->Decrypt(SharedUtil::Base64Decode(message_buffer));
                jsoncons::json json = jsoncons::json::parse(decrypted_buffer);
                HandleIncomingPacket(json);
                m_PendingResponses.insert_or_assign((eAtomicPacket)json["type"].as<int>(), json);
                SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - handled incoming packet type=%d", json["type"].as<int>());
            }
            break;
        }

        case ix::WebSocketMessageType::Close:
        {
            SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - Close");
            m_bNetworkJoined = false;
            m_bConnected = false;
            SharedUtil::AddDebugLog("WebSocket Closed: %s | Remote: %d (%d)", Message->closeInfo.reason.empty() ? "<empty>" : Message->closeInfo.reason.c_str(),
                                    Message->closeInfo.remote ? 1 : 0, Message->closeInfo.code);
            g_pAtomicAntiCheat->GetGuardManager()->StopGuards();
            break;
        }

        case ix::WebSocketMessageType::Error:
            SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - Error");
            SharedUtil::AddDebugLog("WebSocket Error: %s (\"%s\", %d | Decompression Error: %d)", Message->errorInfo.reason.c_str(), Message->str.c_str(),
                                    Message->errorInfo.http_status, Message->errorInfo.decompressionError);
            m_pWebSocket->close();
            m_bNetworkJoined = false;
            g_pAtomicAntiCheat->GetGuardManager()->StopGuards();
            // Reconnect();
            break;
    }
    SharedUtil::AddDebugLog("CAtomicNetwork::OnReceivePacket - exit");
}

void CAtomicNetwork::HandleIncomingPacket(jsoncons::json Packet)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - entered");
    if (!Packet.contains("type"))
    {
        SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - no type field");
        return;
    }

    int iPacketID = Packet["type"].as<int>();
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - type=%d", iPacketID);
    switch ((eAtomicPacket)iPacketID)
    {
        case eAtomicPacket::REQUEST_SCREENSHOT:
            HandleRequestScreenshot(Packet);
            SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - REQUEST_SCREENSHOT handled");
            break;
        case eAtomicPacket::ENGINE_SHUTDOWN:
            HandleEngineShutdown();
            SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - ENGINE_SHUTDOWN handled");
            break;
        case eAtomicPacket::REQUEST_DEBUG_LOGS:
            HandleUploadDebugLogs(Packet);
            SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - REQUEST_DEBUG_LOGS handled");
            break;
        case eAtomicPacket::REQUEST_FILE_UPLOAD:
            HandleFileUpload(Packet);
            SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - REQUEST_FILE_UPLOAD handled");
            break;
    }
    SharedUtil::AddDebugLog("CAtomicNetwork::HandleIncomingPacket - exit");
}

void CAtomicNetwork::RequestFileUpload(std::string strFilePath, std::string strFileHash)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::RequestFileUpload - entered");
    SharedUtil::AddDebugLog("Requesting File Hash...");
    jsoncons::json request = jsoncons::json::object();

    if (strFileHash.empty())
    {
        SharedUtil::AddDebugLog("Failed to request file hash");
        return;
    }

    request["filehash"] = strFileHash;
    request["filepath"] = strFilePath;

    SharedUtil::AddDebugLog("File Hash Requested Successfuly");

    SendPacket(REQUEST_FILEHASH, request);
    SharedUtil::AddDebugLog("CAtomicNetwork::RequestFileUpload - sent file hash request");
}

void CAtomicNetwork::Disconnect(std::string strReason)
{
    SharedUtil::AddDebugLog("CAtomicNetwork::Disconnect - entered reason='%s'", strReason.c_str());
    if (m_pWebSocket->getReadyState() == ix::ReadyState::Open)
    {
        m_pWebSocket->stop(ix::WebSocketCloseConstants::kNormalClosureCode, strReason);
        m_bNetworkJoined = false;
        m_PendingResponses.clear();
        SharedUtil::AddDebugLog("Disconnect - %s", strReason.c_str());
    }
    SharedUtil::AddDebugLog("CAtomicNetwork::Disconnect - exit");
}
