#include "StdInc.h"

#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

using jsoncons::json;

CAtomicHWID::CAtomicHWID()
{
}

CAtomicHWID::~CAtomicHWID()
{
}

std::string CAtomicHWID::RunPowerShellCommand(const std::string& cmd)
{
    // Runs a PowerShell command and captures stdout. Simple, dirty, but effective when
    // native TPM APIs are too heavy to embed. Caller must ensure input is trusted.
    std::string fullCmd = "powershell -NoProfile -NonInteractive -Command \"" + cmd + "\"";

    FILE* pipe = _popen(fullCmd.c_str(), "rt");
    if (!pipe)
        return "";

    char               buffer[512];
    std::ostringstream output;
    while (fgets(buffer, sizeof(buffer), pipe))
    {
        output << buffer;
    }
    _pclose(pipe);
    return output.str();
}

std::map<std::string, std::string> CAtomicHWID::QueryWMI(const std::string& wmiClass, const std::vector<std::string>& properties)
{
    std::map<std::string, std::string> result;

    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool comInited = SUCCEEDED(hres);

    IWbemLocator*  pLoc = nullptr;
    IWbemServices* pSvc = nullptr;

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres))
    {
        if (comInited)
            CoUninitialize();
        return result;
    }

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"),            // WMI namespace
                               NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres))
    {
        pLoc->Release();
        if (comInited)
            CoUninitialize();
        return result;
    }

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    std::string           query = "SELECT * FROM " + wmiClass;
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        if (comInited)
            CoUninitialize();
        return result;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn)
            break;

        for (const auto& prop : properties)
        {
            VARIANT vtProp;
            hr = pclsObj->Get(bstr_t(prop.c_str()), 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr))
            {
                if (vtProp.vt == VT_BSTR && vtProp.bstrVal)
                {
                    result[prop] = _bstr_t(vtProp.bstrVal);
                }
                else if (vtProp.vt == VT_NULL)
                {
                    result[prop] = "";
                }
                else
                {
                    // try to convert other types to string
                    _bstr_t b(_com_util::ConvertBSTRToString(vtProp.bstrVal));
                    result[prop] = (const char*)b;
                }
                VariantClear(&vtProp);
            }
        }

        pclsObj->Release();
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    if (comInited)
        CoUninitialize();

    return result;
}

std::map<std::string, std::string> CAtomicHWID::CollectTPM()
{
    std::map<std::string, std::string> out;

    // Attempt #1: use PowerShell Get-Tpm to read some TPM properties.
    // This is the pragmatic user-mode approach if embedding TBS/NCrypt code is undesirable.
    try
    {
        std::string ps = "Get-Tpm | ConvertTo-Json -Depth 4";
        std::string res = RunPowerShellCommand(ps);
        if (!res.empty())
        {
            out["PowerShell_GetTpm"] = res;
        }
    }
    catch (...)
    {
    }

    // Attempt #2: try to read endorsement key info via PowerShell (Get-TpmEndorsementKeyInfo is available on newer Windows)
    try
    {
        std::string ps = "Try { Get-TpmEndorsementKeyInfo | ConvertTo-Json -Depth 4 } Catch { \"NO_EK_INFO\" }";
        std::string res = RunPowerShellCommand(ps);
        if (!res.empty())
            out["PowerShell_GetTpmEndorsementKeyInfo"] = res;
    }
    catch (...)
    {
    }

    // Note: a native implementation using TBS/NCrypt to query the EK public key and TPM PCRs would be more robust
    // but also much larger. The PowerShell approach provides a pragmatic start in user-mode.

    return out;
}

std::map<std::string, std::string> CAtomicHWID::CollectGPU()
{
    std::map<std::string, std::string> out;

    // First try NVML dynamically
    if (TryGetNvidiaGpuUUIDs(out))
    {
        return out;
    }

    // Fallback: query WMI Win32_VideoController
    auto wmi = QueryWMI("Win32_VideoController", {"Name", "PNPDeviceID", "AdapterCompatibility", "DriverVersion"});
    for (auto& p : wmi)
        out["GPU_" + p.first] = p.second;

    return out;
}

bool CAtomicHWID::TryGetNvidiaGpuUUIDs(std::map<std::string, std::string>& out)
{
    // Attempt to load nvml.dll at runtime and use nvmlDeviceGetUUID.
    // This will only work on machines with the NVIDIA driver/nvml present.

    HMODULE h = LoadLibraryA("nvml.dll");
    if (!h)
        return false;

    using nvmlInit_t = int (*)();
    using nvmlShutdown_t = int (*)();
    using nvmlDeviceGetCount_t = int (*)(int*);
    using nvmlDeviceGetHandleByIndex_t = int (*)(int, void**);
    using nvmlDeviceGetUUID_t = int (*)(void*, char*, int);

    nvmlInit_t                   nvmlInit = (nvmlInit_t)GetProcAddress(h, "nvmlInit_v2");
    nvmlShutdown_t               nvmlShutdown = (nvmlShutdown_t)GetProcAddress(h, "nvmlShutdown");
    nvmlDeviceGetCount_t         nvmlDeviceGetCount = (nvmlDeviceGetCount_t)GetProcAddress(h, "nvmlDeviceGetCount_v2");
    nvmlDeviceGetHandleByIndex_t nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(h, "nvmlDeviceGetHandleByIndex_v2");
    nvmlDeviceGetUUID_t          nvmlDeviceGetUUID = (nvmlDeviceGetUUID_t)GetProcAddress(h, "nvmlDeviceGetUUID");

    if (!nvmlInit || !nvmlShutdown || !nvmlDeviceGetCount || !nvmlDeviceGetHandleByIndex || !nvmlDeviceGetUUID)
    {
        FreeLibrary(h);
        return false;
    }

    if (nvmlInit() != 0)
    {
        FreeLibrary(h);
        return false;
    }

    int count = 0;
    if (nvmlDeviceGetCount(&count) == 0)
    {
        for (int i = 0; i < count; ++i)
        {
            void* handle = nullptr;
            if (nvmlDeviceGetHandleByIndex(i, &handle) == 0)
            {
                char uuid[80] = {0};
                if (nvmlDeviceGetUUID(handle, uuid, sizeof(uuid)) == 0)
                {
                    out["NV_GPU_" + std::to_string(i) + "_UUID"] = uuid;
                }
            }
        }
    }

    nvmlShutdown();
    FreeLibrary(h);
    return !out.empty();
}

std::map<std::string, std::string> CAtomicHWID::QuerySMBIOSViaWmi()
{
    std::map<std::string, std::string> out;
    auto                               bios = QueryWMI("Win32_BIOS", {"Manufacturer", "SMBIOSBIOSVersion", "ReleaseDate", "SerialNumber", "Version"});
    for (auto& p : bios)
        out["BIOS_" + p.first] = p.second;

    auto base = QueryWMI("Win32_BaseBoard", {"Manufacturer", "Product", "SerialNumber", "Version"});
    for (auto& p : base)
        out["BOARD_" + p.first] = p.second;

    auto sys = QueryWMI("Win32_ComputerSystemProduct", {"UUID", "Name", "Vendor", "Version"});
    for (auto& p : sys)
        out["SYSTEM_" + p.first] = p.second;

    auto mem = QueryWMI("Win32_PhysicalMemory", {"SerialNumber", "PartNumber", "Manufacturer", "Capacity"});
    for (auto& p : mem)
        out["MEM_" + p.first] = p.second;            // Note: may overwrite, but keeps keys

    return out;
}

std::map<std::string, std::string> CAtomicHWID::QueryPhysicalDisks()
{
    std::map<std::string, std::string> out;
    // Query WMI Win32_DiskDrive for Model, SerialNumber (SerialNumber may be in Win32_PhysicalMedia)
    auto dd = QueryWMI("Win32_DiskDrive", {"Model", "InterfaceType", "PNPDeviceID"});
    for (auto& p : dd)
        out["DISK_" + p.first] = p.second;

    // Also try Win32_PhysicalMedia for SerialNumber
    auto pm = QueryWMI("Win32_PhysicalMedia", {"Tag", "SerialNumber"});
    for (auto& p : pm)
        out["DISK_PHYS_" + p.first] = p.second;

    return out;
}

std::map<std::string, std::string> CAtomicHWID::CollectDisks()
{
    return QueryPhysicalDisks();
}

std::map<std::string, std::string> CAtomicHWID::CollectSMBIOS()
{
    return QuerySMBIOSViaWmi();
}

std::string CAtomicHWID::GetCPUSerial()
{
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
        return "<unkown>";            // Program has failed.

    if (FAILED(hres))
    {
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    IWbemServices* pSvc = NULL;

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"),            // Object path of WMI namespace
                               NULL,                               // User name. NULL = current user
                               NULL,                               // User password. NULL = current
                               0,                                  // Locale. NULL indicates current
                               NULL,                               // Security flags.
                               0,                                  // Authority (e.g. Kerberos)
                               0,                                  // Context object
                               &pSvc                               // pointer to IWbemServices proxy
    );

    if (FAILED(hres))
    {
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    hres = CoSetProxyBlanket(pSvc,                                   // Indicates the proxy to set
                             RPC_C_AUTHN_WINNT,                      // RPC_C_AUTHN_xxx
                             RPC_C_AUTHZ_NONE,                       // RPC_C_AUTHZ_xxx
                             NULL,                                   // Server principal name
                             RPC_C_AUTHN_LEVEL_CALL,                 // RPC_C_AUTHN_LEVEL_xxx
                             RPC_C_IMP_LEVEL_IMPERSONATE,            // RPC_C_IMP_LEVEL_xxx
                             NULL,                                   // client identity
                             EOAC_NONE                               // proxy capabilities
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    std::string strCPUID;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr))
        {
            strCPUID = static_cast<const char*>(_bstr_t(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return strCPUID;
}

jsoncons::json CAtomicHWID::CollectAllAsJson()
{
    json root = json::object();

    auto tpm = CollectTPM();
    json tpmj = json::object();
    for (auto& p : tpm)
        tpmj[p.first] = p.second;
    root["TPM"] = tpmj;

    root["cpuid"] = GetCPUSerial();

    auto gpu = CollectGPU();
    json gpj = json::object();
    for (auto& p : gpu)
        gpj[p.first] = p.second;
    root["GPU"] = gpj;

    auto disks = CollectDisks();
    json dj = json::object();
    for (auto& p : disks)
        dj[p.first] = p.second;
    root["Disks"] = dj;

    auto smbios = CollectSMBIOS();
    json sj = json::object();
    for (auto& p : smbios)
        sj[p.first] = p.second;
    root["SMBIOS"] = sj;

    return root;
}