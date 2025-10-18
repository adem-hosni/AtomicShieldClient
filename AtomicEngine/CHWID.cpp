#include "StdInc.h"
#include <fstream>
#include <shlobj.h>
#include <Lmcons.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <sstream>
#include <iomanip>
#pragma comment(lib, "wbemuuid.lib")
#include "LiteRegedit/LiteRegedit.h"

CHWID* g_pHWID;

CHWID::CHWID()
{
    SharedUtil::AddDebugLog("CHWID::CHWID - ctor entered");
}

CHWID::~CHWID()
{
    SharedUtil::AddDebugLog("CHWID::~CHWID - dtor entered");
}

jsoncons::json CHWID::GetExtraData()
{
    SharedUtil::AddDebugLog("CHWID::GetExtraData - entered");
    jsoncons::json json = jsoncons::json::object();

    SharedUtil::AddDebugLog("CHWID::GetExtraData - returning empty object");
    return json;
}

jsoncons::json CHWID::GetMonitorSerial()
{
    SharedUtil::AddDebugLog("CHWID::GetMonitorSerial - entered");
    jsoncons::json arr = jsoncons::json::array();
    SharedUtil::AddDebugLog("CHWID::GetMonitorSerial - returning empty array");
    return arr;
}

std::string CHWID::GetWindowsUsername()
{
    SharedUtil::AddDebugLog("CHWID::GetWindowsUsername - entered");
    char  szUsername[UNLEN + 1];
    DWORD dwUsernameLength = UNLEN + 1;

    if (GetUserName(szUsername, &dwUsernameLength))
    {
        SharedUtil::AddDebugLog("CHWID::GetWindowsUsername - success username='%s'", szUsername);
        return szUsername;
    }

    SharedUtil::AddDebugLog("CHWID::GetWindowsUsername - failed to get username");
    return "<unkown>";
}

std::string CHWID::GetSteamID()
{
    SharedUtil::AddDebugLog("CHWID::GetSteamID - entered");
    HKEY           hKey = nullptr;
    const wchar_t* subKey = L"Software\\Valve\\Steam\\ActiveProcess";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
        SharedUtil::AddDebugLog("CHWID::GetSteamID - Registry open failed for '%ls'", subKey);
        return "";
    }
    SharedUtil::AddDebugLog("CHWID::GetSteamID - opened registry key '%ls'", subKey);

    DWORD activeUser = 0;
    DWORD type = 0;
    DWORD size = sizeof(activeUser);

    if (RegQueryValueExW(hKey, L"ActiveUser", nullptr, &type, reinterpret_cast<LPBYTE>(&activeUser), &size) != ERROR_SUCCESS || type != REG_DWORD)
    {
        RegCloseKey(hKey);
        SharedUtil::AddDebugLog("CHWID::GetSteamID - ActiveUser not found or wrong type in registry");
        return "";
    }

    RegCloseKey(hKey);
    SharedUtil::AddDebugLog("CHWID::GetSteamID - ActiveUser value = %u", activeUser);

    if (activeUser == 0)
    {
        SharedUtil::AddDebugLog("CHWID::GetSteamID - ActiveUser == 0 (Steam probably not running)");
        return "";
    }

    unsigned long long steam64 = 76561197960265728ULL + static_cast<unsigned long long>(activeUser);
    SharedUtil::AddDebugLog("CHWID::GetSteamID - computed steam64 = %llu", steam64);

    std::ostringstream oss;
    oss << "steam:" << std::hex << std::nouppercase << steam64;
    std::string result = oss.str();
    SharedUtil::AddDebugLog("CHWID::GetSteamID - returning '%s'", result.c_str());

    return result;
}

std::string CHWID::GetMotherBoardSerial()
{
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - entered");
    HRESULT hr;

    // Initialize COM library
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - CoInitializeEx failed hr=0x%08x", hr);
        std::cout << "Failed to initialize COM library." << std::endl;
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - COM initialized");

    // Obtain WMI locator
    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - CoCreateInstance(WbemLocator) failed hr=0x%08x", hr);
        std::cout << "Failed to create IWbemLocator object." << std::endl;
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - IWbemLocator created");

    // Connect to WMI
    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - ConnectServer failed hr=0x%08x", hr);
        std::cout << "Could not connect to WMI." << std::endl;
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - connected to ROOT\\CIMV2");

    // Set security levels on the proxy
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - CoSetProxyBlanket failed hr=0x%08x", hr);
        std::cout << "Could not set proxy blanket." << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - proxy blanket set");

    // Query motherboard serial number
    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - ExecQuery failed hr=0x%08x", hr);
        std::cout << "Query for motherboard serial number failed." << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - query executed");

    std::string       strMotherBoardSerial = "<unkown>";
    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hrNext = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - Next returned hr=0x%08x uReturn=%u", hrNext, uReturn);
        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtSerialNumber;
        hr = pclsObj->Get(L"SerialNumber", 0, &vtSerialNumber, 0, 0);
        if (SUCCEEDED(hr) && vtSerialNumber.vt == VT_BSTR && vtSerialNumber.bstrVal != nullptr)
        {
            strMotherBoardSerial = static_cast<const char*>(_bstr_t(vtSerialNumber.bstrVal));
            SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - SerialNumber='%s'", strMotherBoardSerial.c_str());
            VariantClear(&vtSerialNumber);
        }
        else
        {
            SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - failed to get SerialNumber hr=0x%08x vt.vt=%u", hr, vtSerialNumber.vt);
        }
        pclsObj->Release();
    }

    // Cleanup
    if (pEnumerator)
        pEnumerator->Release();
    if (pSvc)
        pSvc->Release();
    if (pLoc)
        pLoc->Release();
    CoUninitialize();
    SharedUtil::AddDebugLog("CHWID::GetMotherBoardSerial - cleanup done, returning '%s'", strMotherBoardSerial.c_str());

    return strMotherBoardSerial;
}

jsoncons::json CHWID::GetDisksSerialNumber()
{
    SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - entered");
    // Get logical drives
    DWORD drives = GetLogicalDrives();
    SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - GetLogicalDrives returned 0x%08x", drives);
    if (drives == 0)
    {
        SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - no drives found, returning empty array");
        return jsoncons::json::array();
    }

    // Create a JSON array to hold the results
    jsoncons::json drive_info = jsoncons::json::array();

    char szDrive[] = "A:\\";
    for (char letter = 'A'; letter <= 'Z'; ++letter)
    {
        if (drives & (1 << (letter - 'A')))
        {
            szDrive[0] = letter;
            SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - examining drive %c:", letter);

            char  szVolumeName[MAX_PATH + 1] = {0};
            char  szFileSystemName[MAX_PATH + 1] = {0};
            DWORD dwSerialNumber = 0, dwMaxComponentLength = 0, dwFileSystemFlags = 0;

            if (GetVolumeInformationA(szDrive, szVolumeName, sizeof(szVolumeName), &dwSerialNumber, &dwMaxComponentLength, &dwFileSystemFlags, szFileSystemName,
                                      sizeof(szFileSystemName)))
            {
                std::ostringstream serialStream;
                serialStream << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << dwSerialNumber;
                std::string serialStr = serialStream.str();

                SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - drive=%c volume='%s' fs='%s' serial=0x%08X (%s)", letter, szVolumeName, szFileSystemName,
                                        dwSerialNumber, serialStr.c_str());

                drive_info.push_back(serialStr);
            }
            else
            {
                SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - GetVolumeInformationA failed for %c:", letter);
            }
        }
    }

    SharedUtil::AddDebugLog("CHWID::GetDisksSerialNumber - completed, returning %zu entries", drive_info.size());
    return drive_info;
}

std::string CHWID::GetCPUsSerials()
{
    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - entered");
    HRESULT hres;

    // Initialize COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - CoInitializeEx failed hr=0x%08x", hres);
        return "<unkown>";            // Program has failed.
    }
    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - COM initialized");

    // Obtain the initial locator to WMI
    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - CoCreateInstance(WbemLocator) failed hr=0x%08x", hres);
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }
    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - IWbemLocator created");

    IWbemServices* pSvc = NULL;

    // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - ConnectServer failed hr=0x%08x", hres);
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }
    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - connected to WMI");

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - CoSetProxyBlanket failed hr=0x%08x", hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }
    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - proxy blanket set");

    // Use the IWbemServices pointer to make requests of WMI
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - ExecQuery failed hr=0x%08x", hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }
    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - query executed");

    // Secure the enumeration
    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;
    HRESULT           hr;

    // JSON array to hold all processor information
    std::string strCPUID;

    while (pEnumerator)
    {
        HRESULT hrNext = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - Next returned hr=0x%08x uReturn=%u", hrNext, uReturn);
        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the ProcessorId property
        hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr)
        {
            strCPUID = static_cast<const char*>(_bstr_t(vtProp.bstrVal));
            SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - ProcessorId='%s'", strCPUID.c_str());
        }
        else
        {
            SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - failed to get ProcessorId hr=0x%08x vt.vt=%u", hr, vtProp.vt);
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    if (pSvc)
        pSvc->Release();
    if (pLoc)
        pLoc->Release();
    if (pEnumerator)
        pEnumerator->Release();
    CoUninitialize();

    SharedUtil::AddDebugLog("CHWID::GetCPUsSerials - returning '%s'", strCPUID.c_str());
    return strCPUID;
}

std::string CHWID::GetBIOSVersion()
{
    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - entered");
    HRESULT hr;

    // Initialize COM library
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - CoInitializeEx failed hr=0x%08x", hr);
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - COM initialized");

    // Obtain WMI locator
    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - CoCreateInstance(WbemLocator) failed hr=0x%08x", hr);
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - IWbemLocator created");

    // Connect to WMI
    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - ConnectServer failed hr=0x%08x", hr);
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - connected to WMI");

    // Set security levels on the proxy
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - CoSetProxyBlanket failed hr=0x%08x", hr);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - proxy blanket set");

    // Query BIOS version
    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_BIOS"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - ExecQuery failed hr=0x%08x", hr);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - query executed");

    std::string       strBIOSVersion = "<unkown>";
    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hrNext = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - Next returned hr=0x%08x uReturn=%u", hrNext, uReturn);
        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtBIOSVersion;
        hr = pclsObj->Get(L"Version", 0, &vtBIOSVersion, 0, 0);
        if (SUCCEEDED(hr) && vtBIOSVersion.vt == VT_BSTR && vtBIOSVersion.bstrVal != nullptr)
        {
            strBIOSVersion = static_cast<const char*>(_bstr_t(vtBIOSVersion.bstrVal));
            SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - Version='%s'", strBIOSVersion.c_str());
            VariantClear(&vtBIOSVersion);
        }
        else
        {
            SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - failed to get Version hr=0x%08x vt.vt=%u", hr, vtBIOSVersion.vt);
            pclsObj->Release();
            break;
        }

        pclsObj->Release();
    }

    // Cleanup
    if (pEnumerator)
        pEnumerator->Release();
    if (pSvc)
        pSvc->Release();
    if (pLoc)
        pLoc->Release();
    CoUninitialize();

    SharedUtil::AddDebugLog("CHWID::GetBIOSVersion - returning '%s'", strBIOSVersion.c_str());
    return strBIOSVersion;
}

std::string CHWID::GetPNPDeviceID()
{
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - entered");
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - CoInitializeEx failed hr=0x%08x", hres);
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - COM initialized");

    IWbemLocator* pLoc = NULL;

    // Create WMI locator
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - CoCreateInstance(WbemLocator) failed hr=0x%08x", hres);
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - IWbemLocator created");

    IWbemServices* pSvc = NULL;

    // Connect to WMI
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - ConnectServer failed hr=0x%08x", hres);
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - connected to WMI");

    // Set security levels for the WMI connection
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - CoSetProxyBlanket failed hr=0x%08x", hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - proxy blanket set");

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                           &pEnumerator);

    if (FAILED(hres))
    {
        SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - ExecQuery failed hr=0x%08x", hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - query executed");

    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;
    HRESULT           hr;
    std::string       strPNPDeviceID = "<unkown>";

    while (pEnumerator)
    {
        HRESULT hrNext = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - Next returned hr=0x%08x uReturn=%u", hrNext, uReturn);
        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the PNPDeviceID property
        hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr)
        {
            strPNPDeviceID = static_cast<const char*>(_bstr_t(vtProp.bstrVal));
            SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - PNPDeviceID='%s'", strPNPDeviceID.c_str());
        }
        else
        {
            SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - failed to get PNPDeviceID hr=0x%08x vt.vt=%u", hr, vtProp.vt);
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    if (pSvc)
        pSvc->Release();
    if (pLoc)
        pLoc->Release();
    if (pEnumerator)
        pEnumerator->Release();
    CoUninitialize();
    SharedUtil::AddDebugLog("CHWID::GetPNPDeviceID - returning '%s'", strPNPDeviceID.c_str());
    return strPNPDeviceID;
}

std::string CHWID::GetComputerName_()
{
    SharedUtil::AddDebugLog("CHWID::GetComputerName_ - entered");
    char  computerName[MAX_PATH];
    DWORD size = sizeof(computerName) / sizeof(computerName[0]);
    if (!GetComputerName(computerName, &size))
    {
        SharedUtil::AddDebugLog("CHWID::GetComputerName_ - GetComputerName failed");
        return "<NONE>";
    }
    SharedUtil::AddDebugLog("CHWID::GetComputerName_ - name='%s'", computerName);
    return computerName;
}

void CHWID::StoreHWIDCaches(jsoncons::json hwid)
{
    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - entered");
    PWSTR path = nullptr;

    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - writing cpu");
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_CPU", hwid["cpu"].as_string());
    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - writing motherboard_serial");
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_MTS", hwid["motherboard_serial"].as_string());
    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - writing bios");
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_BIOS", hwid["bios"].as_string());
    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - writing pnp_device");
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_PNPDEV", hwid["pnp_device"].as_string());
    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - writing disks");
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_DISKS", hwid["disks"].as_string());

    SharedUtil::AddDebugLog("CHWID::StoreHWIDCaches - completed");
}

jsoncons::json CHWID::LoadHWIDCaches()
{
    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - entered");
    jsoncons::json json;
    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - reading cpu");
    json["cpu"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_CPU");
    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - reading motherboard_serial");
    json["motherboard_serial"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_MTS");
    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - reading bios");
    json["bios"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_BIOS");
    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - reading pnp_device");
    json["pnp_device"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_PNPDEV");
    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - reading disks");
    json["disks"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_DISKS");

    SharedUtil::AddDebugLog("CHWID::LoadHWIDCaches - completed");
    return json;
}

bool CHWID::WriteADS(std::string strPath, std::string strStreamName, std::string strData)
{
    SharedUtil::AddDebugLog("CHWID::WriteADS - entered path='%s' stream='%s' data_length=%zu", strPath.c_str(), strStreamName.c_str(), strData.size());
    if (strPath.empty())
    {
        SharedUtil::AddDebugLog("CHWID::WriteADS - empty path, aborting");
        return false;
    }

    if (strPath.back() == '\\')
        strPath.pop_back();

    std::string strADSPath = strPath + ":" + strStreamName;
    SharedUtil::AddDebugLog("CHWID::WriteADS - ADS path='%s'", strADSPath.c_str());
    std::ofstream ads(strADSPath, std::ios::out | std::ios::trunc);
    if (!ads)
    {
        SharedUtil::AddDebugLog("CHWID::WriteADS - Unable to open ads '%s' for writing", strADSPath.c_str());
        return false;
    }

    ads << strData;
    ads.close();
    SharedUtil::AddDebugLog("CHWID::WriteADS - wrote %zu bytes to '%s'", strData.size(), strADSPath.c_str());
    return true;
}

std::string CHWID::ReadADS(std::string strPath, std::string strStreamName)
{
    SharedUtil::AddDebugLog("CHWID::ReadADS - entered path='%s' stream='%s'", strPath.c_str(), strStreamName.c_str());
    if (strPath.empty())
    {
        SharedUtil::AddDebugLog("CHWID::ReadADS - empty path, returning empty string");
        return "";
    }

    if (strPath.back() == '\\')
        strPath.pop_back();

    std::string strADSPath = strPath + ":" + strStreamName;
    SharedUtil::AddDebugLog("CHWID::ReadADS - ADS path='%s'", strADSPath.c_str());
    std::ifstream ads(strADSPath, std::ios::binary);
    if (!ads)
    {
        SharedUtil::AddDebugLog("CHWID::ReadADS - Unable to open ads '%s' for reading", strADSPath.c_str());
        return "";
    }
    std::string data((std::istreambuf_iterator<char>(ads)), std::istreambuf_iterator<char>());

    ads.close();
    SharedUtil::AddDebugLog("CHWID::ReadADS - read %zu bytes from '%s'", data.size(), strADSPath.c_str());
    return data;
}