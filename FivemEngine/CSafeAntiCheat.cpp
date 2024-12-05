#include "CSafeAntiCheat.h"
#include "SharedUtil.h"
#include <ctime>

CSafeAntiCheat* g_pSafeAntiCheat = new CSafeAntiCheat();

CSafeAntiCheat::CSafeAntiCheat()
{
    m_pSafeNetwork = new CSafeNetwork();
    m_Timing = {};
}

CSafeAntiCheat::~CSafeAntiCheat()
{
    if (m_pSafeNetwork)
        delete m_pSafeNetwork;
}

void CSafeAntiCheat::DoPulse()
{
    CSafeNetwork* pEagleNetwork = g_pSafeAntiCheat->GetNetwork();
    while (true)
    {
        pEagleNetwork->DoPulse();
        int iProcessID = SharedUtil::GetFivemProcessID();
        if (!iProcessID)
        {
            Sleep(50);
            continue;
        }

        long long     llCurrentTime = time(NULL);
        STiming&      Timing = g_pSafeAntiCheat->GetTiming();

        if (!g_pMemoryScanner->IsAttached())
            g_pMemoryScanner->Attach(iProcessID);

        if (llCurrentTime - Timing.llLastMemoryScan > GAME_MEMORY_SCAN_INTERVAL)
        {
            g_pMemoryScanner->ScanStrings(pEagleNetwork->GetSignatures());

            std::vector<std::string> vSignatures = g_pMemoryScanner->GetDetectedSignatures();
            unsigned int             uiScanResult = vSignatures.size();

            // New Signature Found ?
            if (uiScanResult != g_pMemoryScanner->GetLatestScanResult())
            {
                g_pMemoryScanner->UpdateLatestScanResult(uiScanResult);
                jsoncons::json RequestData = jsoncons::json::object();
                RequestData["signatures"] = vSignatures;
                pEagleNetwork->SendPacket(eEaglePacketID::MALICIOUS_SIGNATURE_DETECTION, RequestData);
            }
            Timing.llLastMemoryScan = llCurrentTime;
        }
    }
}