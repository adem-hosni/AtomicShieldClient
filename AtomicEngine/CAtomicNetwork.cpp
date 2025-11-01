#include "CAtomicAntiCheat.h"
#include <ixwebsocket/IXHttpClient.h>
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
    m_pWebSocket = new ix::WebSocket();
}

CAtomicNetwork::~CAtomicNetwork()
{
}

bool CAtomicNetwork::Connect()
{
    delete m_pWebSocket;
    m_pWebSocket = new ix::WebSocket();

    m_bNetworkJoined = false;

    SharedUtil::AddDebugLog("Initializing network system...");
    ix::initNetSystem();

    m_pWebSocket->setUrl(m_strServerEndPoint + "/c/atomicshieldagent/");

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

    return result.success;
}

void CAtomicNetwork::SendPacket(eAtomicPacket PacketID, jsoncons::json Data, bool bHighPriority)
{
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet type and the unix timestamp
    PacketJson["type"] = (unsigned short)PacketID;
    PacketJson["ut"] = Utils::FastEpochSeconds();

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to master server
    std::string buffer = g_pAtomicCore->Encrypt(PacketJson.to_string());

    if (bHighPriority)
    {
        m_pWebSocket->send(SharedUtil::Base64Encode(buffer));
        return;
    }
    m_vPendingPackets.push(SharedUtil::Base64Encode(buffer));
}

void CAtomicNetwork::OnConnect()
{
    SharedUtil::AddDebugLog("Connected to the AtomicShield Server!");
    if (!g_pAtomicAntiCheat->GetNetwork()->JoinNetwork())
    {
        SharedUtil::AddDebugLog("Failed to join the AtomicShield Network, shutting down the engine...");
        FreeModule(GetModuleHandle(NULL));
        return;
    }
}

void CAtomicNetwork::StaticPulse(void* pContext)
{
    CAtomicNetwork* pNetwork = reinterpret_cast<CAtomicNetwork*>(pContext);
    pNetwork->DoPulse();
}

jsoncons::json CAtomicNetwork::WaitReponse(eAtomicPacket PacketID)
{
    while (m_PendingResponses.find(PacketID) == m_PendingResponses.end())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    jsoncons::json Response = m_PendingResponses[PacketID];

    // Check if the unix timestamp received is tampered
    // if (Utils::FastEpochSeconds() - Response["ut"].as<DWORD>() >= 20)
    //{
    //    __fastfail(0);
    //    // Return an empty data to crash the engine if the __fastfail was tampered
    //    jsoncons::json j;
    //    return j;
    //}

    m_PendingResponses.erase(PacketID);
    return Response;
}

std::string CAtomicNetwork::GetIPAddressChain()
{
    struct IPApi
    {
        std::wstring host;
        std::wstring path;
        bool         json;            // if true, extract "ip" from {"ip": "..."}
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
            continue;
        }

        std::string extractedIp;
        if (api.json)
        {
            size_t ipKey = response.find("\"ip\"");
            if (ipKey != std::string::npos)
            {
                size_t quote1 = response.find('\"', ipKey + 4);
                size_t quote2 = response.find('\"', quote1 + 1);
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
            collectedIPs.push_back(extractedIp);
        }
        else
        {
            // SharedUtil::AddDebugLog("[HTTP] Failed to parse IP from %ws response: %s", api.host.c_str(), response.c_str());
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

    return combined;
}

bool CAtomicNetwork::JoinNetwork()
{
    jsoncons::json RequestData;
    RequestData["ip"] = GetIPAddressChain();
    SharedUtil::AddDebugLog("Connecting With %s", RequestData["ip"].as_string().c_str());

    SharedUtil::AddDebugLog("Joining Network...");

    jsoncons::json RequestHWID = g_pAtomicAntiCheat->GetAtomicHWID()->CollectAllAsJson();
    SharedUtil::AddDebugLog("Collected HWID data. %s", RequestHWID.to_string().c_str());

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
    RequestHWID["extra"] = jsoncons::json::object();

    RequestData["hwid"] = RequestHWID;
    RequestData["cache"] = g_pAtomicAntiCheat->GetCurrentHWIDCache();
    RequestData["engine_type"] = 2;                                     // FiveM
    RequestData["build_timestamp"] = CLIENT_BUILD_TIMESTAMP;            // Some players uses an old engine

    SendPacket(eAtomicPacket::NETWORK_JOIN, RequestData, true);

    SharedUtil::AddDebugLog("Waiting for network join response...");
    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    m_bNetworkJoined = Response["success"].as_bool();

    if (m_bNetworkJoined)
    {
        //g_pHWID->StoreHWIDCaches(RequestHWID);

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

    return true;
}

void CAtomicNetwork::HandleRequestScreenshot(jsoncons::json& Packet)
{
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
}

void CAtomicNetwork::HandleEngineShutdown(jsoncons::json& Packet)
{
    std::string strReason = Packet["reason"].as_string();
    if (m_bNetworkJoined)
    {
        g_pAtomicAntiCheat->Shutdown(strReason.length() > 0 ? strReason : "Remote shutdown requested");
    }
    else
    {
        SharedUtil::AddDebugLog("Engine shutdown requested but network not joined yet.");
    }
}

void CAtomicNetwork::HandleUploadDebugLogs(jsoncons::json& Packet)
{
    jsoncons::json response = jsoncons::json::object();
    response["request_id"] = Packet.contains("request_id") ? Packet["request_id"].as_string() : SharedUtil::GenerateRandomString(8);

    std::string strLogs;

    response["success"] = SharedUtil::GetDebugLogs(strLogs);
    response["logs"] = strLogs;

    SendPacket(REQUEST_DEBUG_LOGS, response);
}

void CAtomicNetwork::HandleFileUpload(jsoncons::json& Packet)
{
    jsoncons::json response = jsoncons::json::object();

    response["request_id"] = Packet.contains("request_id") ? Packet["request_id"].as_string() : SharedUtil::GenerateRandomString(8);

    std::string strFilePath = Packet["file_path"].as_string();
    if (strFilePath.empty())
    {
        response["success"] = false;
        response["message"] = "File path is empty!";
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
            }
            else
            {
                response["success"] = false;
                response["message"] = "Failed to open file!";
            }
        }
        else
        {
            response["success"] = false;
            response["message"] = "File not found!";
        }
    }
    SendPacket(REQUEST_FILE_UPLOAD, response);
}

void CAtomicNetwork::HandleRunScanners(jsoncons::json& Packet)
{
}

void CAtomicNetwork::Ping(eHeartbeatType HeartbeatType)
{
    if (m_pWebSocket->getReadyState() == ix::ReadyState::Open)
    {
        jsoncons::json body = jsoncons::json::object();
        body["heartbeat_type"] = (unsigned short)HeartbeatType;

        SendPacket(HEARTBEAT, body);
        m_ullLastPingTime = Utils::FastEpochSeconds();
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
                                   std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                   return true;
                               });
            m_vPendingPackets.pop();
        }
    }

    if (Utils::FastEpochSeconds() - m_ullLastPingTime > 5)
    {
        m_pWebSocket->ping("Ping");
        m_ullLastPingTime = Utils::FastEpochSeconds();
    }
}

void CAtomicNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    SharedUtil::AddDebugLog("Websocket Message callback triggered with type %d", (int)Message->type);

    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
        {
            CAtomicThread::Create(&CAtomicNetwork::OnConnect, this);
            break;
        }

        case ix::WebSocketMessageType::Message:
        {
            if (Message->type == ix::WebSocketMessageType::Message)
            {
                std::string    message_buffer = Message->str;
                std::string    decrypted_buffer = g_pAtomicCore->Decrypt(SharedUtil::Base64Decode(message_buffer));
                jsoncons::json json = jsoncons::json::parse(decrypted_buffer);
                HandleIncomingPacket(json);
                m_PendingResponses.insert_or_assign((eAtomicPacket)json["type"].as<int>(), json);
            }
            break;
        }

        case ix::WebSocketMessageType::Close:
        {
            m_bNetworkJoined = false;
            m_bConnected = false;
            SharedUtil::AddDebugLog("WebSocket Closed: %s | Remote: %d (%d)", Message->closeInfo.reason.empty() ? "<empty>" : Message->closeInfo.reason.c_str(),
                                    Message->closeInfo.remote ? 1 : 0, Message->closeInfo.code);
            g_pAtomicAntiCheat->GetGuardManager()->StopGuards();
            break;
        }

        case ix::WebSocketMessageType::Error:
            SharedUtil::AddDebugLog("WebSocket Error: %s (\"%s\", %d | Decompression Error: %d)", Message->errorInfo.reason.c_str(), Message->str.c_str(),
                                    Message->errorInfo.http_status, Message->errorInfo.decompressionError);
            m_pWebSocket->close();
            m_bNetworkJoined = false;
            g_pAtomicAntiCheat->GetGuardManager()->StopGuards();
            break;
    }
}

void CAtomicNetwork::HandleIncomingPacket(jsoncons::json Packet)
{
    if (!Packet.contains("type"))
        return;

    int iPacketID = Packet["type"].as<int>();

    SharedUtil::AddDebugLog("Handling incoming packet of type %d", iPacketID);

    switch ((eAtomicPacket)iPacketID)
    {
        case eAtomicPacket::REQUEST_SCREENSHOT:
            HandleRequestScreenshot(Packet);
            break;
        case eAtomicPacket::ENGINE_SHUTDOWN:
            HandleEngineShutdown(Packet);
            break;
        case eAtomicPacket::REQUEST_DEBUG_LOGS:
            HandleUploadDebugLogs(Packet);
            break;
        case eAtomicPacket::REQUEST_FILE_UPLOAD:
            HandleFileUpload(Packet);
            break;
        case eAtomicPacket::RELOAD_ENGINE:
            HandleReloadEngine(Packet);
            break;
    }
}

void CAtomicNetwork::HandleReloadEngine(jsoncons::json& Packet)
{
    SharedUtil::AddDebugLog("Handling engine reload request...");
    jsoncons::json response = jsoncons::json::object();
    response["request_id"] = Packet.contains("request_id") ? Packet["request_id"].as_string() : SharedUtil::GenerateRandomString(8);

    response["success"] = false;
    response["message"] = "";

    SharedUtil::AddDebugLog("Allocating http client...");
    ix::HttpClient client;

    std::string endpoint = m_strServerEndPoint + "/resources/scan/fivem";
    std::string body;

    SharedUtil::AddDebugLog("Allocating http request args pointer...");
    ix::HttpRequestArgsPtr pRequestArgs = std::make_shared<ix::HttpRequestArgs>();
    pRequestArgs->compress = true;
    pRequestArgs->extraHeaders["User-Agent"] = "AtomicShield/Engine";

    SharedUtil::AddDebugLog("Downloading engine from %s", endpoint.c_str());
    ix::HttpResponsePtr pResponse = client.post(endpoint, body, pRequestArgs);

    SharedUtil::AddDebugLog("got engine response code: %d", pResponse->statusCode);
    if (pResponse->statusCode != 200)
    {
        SharedUtil::AddDebugLog("Failed to download engine, server responded with code %d | Body: %s", pResponse->statusCode, pResponse->body.c_str());
        response["success"] = false;
        response["message"] = "Failed to download engine!";
        return;
    }

    std::string strResponseBody = pResponse->body;
    if (strResponseBody.empty() || strResponseBody.length() < 50)
    {
        SharedUtil::AddDebugLog(
            strResponseBody.empty() ? "Failed to download engine, server responded with empty body" : "Failed to download engine, got invalid response %s",
            strResponseBody.empty() ? "" : strResponseBody.c_str());
        response["success"] = false;
        response["message"] = "Invalid server response!";
        return;
    }

    SharedUtil::AddDebugLog("Engine downloaded successfuly");

    std::string strEngineBuffer = g_pAtomicCore->Decrypt(SharedUtil::Base64Decode(strResponseBody));

    SharedUtil::AddDebugLog("Injecting engine into current process...");
    int iInjectionResult = SelfMapModule::MapModule((BYTE*)strEngineBuffer.data(), strEngineBuffer.size(), true, true, true, true, DLL_PROCESS_ATTACH, 0);
    if (iInjectionResult != 0)
    {
        SharedUtil::AddDebugLog("Failed to inject engine, error code %d", iInjectionResult);
        response["success"] = false;
        response["message"] = "Failed to inject engine!";
        return;
    }
    SharedUtil::AddDebugLog("Engine injected successfuly");
}

void CAtomicNetwork::RequestFileUpload(std::string strFilePath, std::string strFileHash)
{
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
}

void CAtomicNetwork::Disconnect(std::string strReason)
{
    if (m_pWebSocket->getReadyState() == ix::ReadyState::Open)
    {
        m_pWebSocket->stop(ix::WebSocketCloseConstants::kNormalClosureCode, strReason);
        m_bNetworkJoined = false;
        m_PendingResponses.clear();
        SharedUtil::AddDebugLog("Disconnect - %s", strReason.c_str());
    }
}
