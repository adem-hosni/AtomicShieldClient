#pragma once
#include "StdInc.h"
#include "jsoncons/json.hpp"

class CEagleAPI
{
public:
    CEagleAPI();
    ~CEagleAPI();

    void DownloadAgentPEB(std::string* buffer);

private:
    std::string PostRequest(const char* szURL, jsoncons::json Data = jsoncons::json(), bool bEncryptRequestBody = true, bool bDecryptRespnseBody = true);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response);
};

extern CEagleAPI* g_pEagleAPI;