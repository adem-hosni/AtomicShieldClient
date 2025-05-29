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

    jsoncons::json GetStatus();
    bool           IsAlreadyConnected();
    bool           IsValidVersion(const char* szVersion);
    void           DownloadEngine(std::string* buffer, SUserData* pUserData);
    bool           UploadCashReport(jsoncons::json Report);

private:
    std::string   PostRequest(const char* szURL, jsoncons::json Data = jsoncons::json(), SUserData* pUserData = nullptr, bool bEncryptRequestBody = true, bool bDecryptRespnseBody = true);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response);
    static int    ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
};

extern CAtomicAPI* g_pAtomicAPI;