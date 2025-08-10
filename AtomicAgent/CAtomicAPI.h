#pragma once
#include "jsoncons/json.hpp"
#include "curl/curl.h"

struct SUserData
{
    float fProgress = 0.f;
};

class CAtomicAPI
{
public:
    CAtomicAPI();
    ~CAtomicAPI();

    void           SetServerEndPoint(const std::string& url) { m_strServerEndPoint = url; }
    jsoncons::json GetStatus();
    bool           IsAlreadyConnected();
    bool           IsValidVersion(const char* szVersion);
    void           DownloadEngine(std::string* buffer, SUserData* pUserData);
    void           DownloadLatestAgent(std::string* buffer);

private:
    std::string   PostRequest(std::string strURL, jsoncons::json Data = jsoncons::json(), SUserData* pUserData = nullptr, bool bEncryptRequestBody = true,
                                   bool bDecryptRespnseBody = true);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response);
    static int    ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

    std::string m_strServerEndPoint;
};

extern CAtomicAPI* g_pAtomicAPI;