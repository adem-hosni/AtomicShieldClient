#include "StdInc.h"
#include <fstream>
#include <shlobj.h>
#include <Lmcons.h>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#include "LiteRegedit/LiteRegedit.h"

CHWID* g_pHWID;

CHWID::CHWID()
{
}

CHWID::~CHWID()
{
}

jsoncons::json CHWID::GetExtraData()
{
    jsoncons::json json = jsoncons::json::object();

    return json;
}

jsoncons::json CHWID::GetMonitorSerial()
{
    return jsoncons::json::array();
}

std::string CHWID::GetWindowsUsername()
{
    char  szUsername[UNLEN + 1];
    DWORD dwUsernameLength = UNLEN + 1;

    if (GetUserName(szUsername, &dwUsernameLength))
    {
        return szUsername;
    }
    return "<unkown>";
}





std::string GetSteamPath()
{
    HKEY   hKey;
    LPCSTR regPath = "Software\\Valve\\Steam";
    LPCSTR valueName = "SteamPath";
    CHAR   path[MAX_PATH];
    DWORD  pathLength = sizeof(path);

    if (RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)path, &pathLength) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return std::string(path);
        }
        RegCloseKey(hKey);
    }
    return "";
}

std::string ExtractSteamIDFromLoginUsers(const std::string& content)
{
    std::istringstream stream(content);
    std::string        line;
    std::string        lastKey;
    bool               mostRecentFound = false;

    while (std::getline(stream, line))
    {
        // trim spaces
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty())
            continue;

        if (line[0] == '\"')
        {
            size_t end = line.find('\"', 1);
            if (end != std::string::npos)
            {
                std::string key = line.substr(1, end - 1);

                // Remember last key (SteamID is a numeric-only key)
                if (key.find_first_not_of("0123456789") == std::string::npos)
                {
                    lastKey = key;
                }

                // If we hit MostRecent = 1, return lastKey
                if (key == "MostRecent" && line.find("\"1\"") != std::string::npos && !lastKey.empty())
                {
                    return lastKey;
                }
            }
        }
    }
    return "";
}

std::string ExtractSteamIDFromConfig(const std::string& content)
{
    std::istringstream stream(content);
    std::string        line;
    std::string        steamID;
    bool               inUsersSection = false;
    int                braceCount = 0;

    while (std::getline(stream, line))
    {
        if (line.find("\"Users\"") != std::string::npos)
        {
            inUsersSection = true;
            continue;
        }

        if (inUsersSection)
        {
            if (line.find('{') != std::string::npos)
                braceCount++;
            if (line.find('}') != std::string::npos)
                braceCount--;

            if (braceCount == 0)
            {
                inUsersSection = false;
                continue;
            }

            size_t start = line.find('\"');
            if (start != std::string::npos)
            {
                size_t end = line.find('\"', start + 1);
                if (end != std::string::npos)
                {
                    steamID = line.substr(start + 1, end - start - 1);
                    if (!steamID.empty() && steamID.find_first_not_of("0123456789") == std::string::npos)
                    {
                        return steamID;
                    }
                }
            }
        }
    }

    return "";
}
std::string DecimalToSteamHex(const std::string& decimalSteamID)
{
    try
    {
        uint64_t          steam64 = std::stoull(decimalSteamID);
        std::stringstream ss;
        ss << "steam:" << std::hex << std::nouppercase << steam64;
        return ss.str();
    }
    catch (...)
    {
        return "";         
    }
}
std::string CHWID::GetSteamID()
{
    std::string steamPath = GetSteamPath();
    if (steamPath.empty())
    {
        std::cerr << "Failed to find Steam installation path." << std::endl;
    }

    std::cout << "Steam path: " << steamPath << std::endl;

    std::string   loginUsersPath = steamPath + "\\config\\loginusers.vdf";
    std::ifstream loginUsersFile(loginUsersPath);

    if (loginUsersFile.is_open())
    {
        std::stringstream buffer;
        buffer << loginUsersFile.rdbuf();
        std::string content = buffer.str();
        loginUsersFile.close();

        std::string steamID = ExtractSteamIDFromLoginUsers(content);
        if (!steamID.empty())
        {
            std::string steamHex = DecimalToSteamHex(steamID);
            std::cout << "Steam ID from loginusers.vdf: " << steamHex << std::endl;
            return steamHex;
        }
    }

    std::string   configPath = steamPath + "\\config\\config.vdf";
    std::ifstream configFile(configPath);

    if (configFile.is_open())
    {
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        std::string content = buffer.str();
        configFile.close();

        std::string steamID = ExtractSteamIDFromConfig(content);
        if (!steamID.empty())
        {
            std::string steamHex = DecimalToSteamHex(steamID);
            std::cout << "Steam ID from loginusers.vdf: " << steamHex << std::endl;
            return steamHex;

        }
    }
}

std::string CHWID::GetMotherBoardSerial()
{
    HRESULT hr;

    // Initialize COM library
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::cout << "Failed to initialize COM library." << std::endl;
        return "<unkown>";
    }

    // Obtain WMI locator
    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr))
    {
        std::cout << "Failed to create IWbemLocator object." << std::endl;
        CoUninitialize();
        return "<unkown>";
    }

    // Connect to WMI
    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr))
    {
        std::cout << "Could not connect to WMI." << std::endl;
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    // Set security levels on the proxy
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        std::cout << "Could not set proxy blanket." << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    // Query motherboard serial number
    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr))
    {
        std::cout << "Query for motherboard serial number failed." << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    std::string       strMotherBoardSerial = "<unkown>";
    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtSerialNumber;
        hr = pclsObj->Get(L"SerialNumber", 0, &vtSerialNumber, 0, 0);
        if (SUCCEEDED(hr))
        {
            strMotherBoardSerial = static_cast<const char*>(_bstr_t(vtSerialNumber.bstrVal));
            VariantClear(&vtSerialNumber);
        }
        pclsObj->Release();
    }

    // Cleanup
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return strMotherBoardSerial;
}

jsoncons::json CHWID::GetDisksSerialNumber()
{
    // Get logical drives
    DWORD drives = GetLogicalDrives();
    if (drives == 0)
        return jsoncons::json::array();

    // Create a JSON array to hold the results
    jsoncons::json drive_info = jsoncons::json::array();

    char szDrive[] = "A:\\";
    for (char letter = 'A'; letter <= 'Z'; ++letter)
    {
        if (drives & (1 << (letter - 'A')))
        {
            szDrive[0] = letter;

            char  szVolumeName[MAX_PATH + 1] = {0};
            char  szFileSystemName[MAX_PATH + 1] = {0};
            DWORD dwSerialNumber = 0, dwMaxComponentLength = 0, dwFileSystemFlags = 0;

            if (GetVolumeInformationA(szDrive, szVolumeName, sizeof(szVolumeName), &dwSerialNumber, &dwMaxComponentLength, &dwFileSystemFlags, szFileSystemName,
                                      sizeof(szFileSystemName)))
            {
                std::ostringstream serialStream;
                serialStream << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << dwSerialNumber;

                drive_info.push_back(serialStream.str());
            }
        }
    }

    return drive_info;
}

std::string CHWID::GetCPUsSerials()
{
    HRESULT hres;

    // Initialize COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
        return "<unkown>";            // Program has failed.

    if (FAILED(hres))
    {
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    // Obtain the initial locator to WMI
    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    IWbemServices* pSvc = NULL;

    // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
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

    // Set security levels on the proxy
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

    // Use the IWbemServices pointer to make requests of WMI
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";            // Program has failed.
    }

    // Secure the enumeration
    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    // JSON array to hold all processor information
    std::string strCPUID;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the ProcessorId property
        hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr))
        {
            strCPUID = static_cast<const char*>(_bstr_t(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return strCPUID;
}

std::string CHWID::GetBIOSVersion()
{
    HRESULT hr;

    // Initialize COM library
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return "<unkown>";
    }

    // Obtain WMI locator
    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr))
    {
        CoUninitialize();
        return "<unkown>";
    }

    // Connect to WMI
    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr))
    {
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    // Set security levels on the proxy
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    // Query BIOS version
    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_BIOS"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    std::string       strBIOSVersion = "<unkown>";
    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtBIOSVersion;
        hr = pclsObj->Get(L"Version", 0, &vtBIOSVersion, 0, 0);
        if (SUCCEEDED(hr))
        {
            strBIOSVersion = static_cast<const char*>(_bstr_t(vtBIOSVersion.bstrVal));
            VariantClear(&vtBIOSVersion);
        }
        else
        {
            return "<unkown>";
        }

        pclsObj->Release();
    }

    // Cleanup
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return strBIOSVersion;
}

std::string CHWID::GetPNPDeviceID()
{
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        return "<unkown>";
    }

    IWbemLocator* pLoc = NULL;

    // Create WMI locator
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        CoUninitialize();
        return "<unkown>";
    }

    IWbemServices* pSvc = NULL;

    // Connect to WMI
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres))
    {
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    // Set security levels for the WMI connection
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                           &pEnumerator);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "<unkown>";
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG             uReturn = 0;
    std::string       strPNPDeviceID = "<unkown>";

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the PNPDeviceID property
        hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr))
        {
            strPNPDeviceID = static_cast<const char*>(_bstr_t(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return strPNPDeviceID;
}

std::string CHWID::GetComputerName_()
{
    char  computerName[MAX_PATH];
    DWORD size = sizeof(computerName) / sizeof(computerName[0]);
    if (!GetComputerName(computerName, &size))
        return "<NONE>";
    return computerName;
}

void CHWID::StoreHWIDCaches(jsoncons::json hwid)
{
    PWSTR       path = nullptr;
    
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_CPU", hwid["cpu"].as_string());
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_MTS", hwid["motherboard_serial"].as_string());
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_BIOS", hwid["bios"].as_string());
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_PNPDEV", hwid["pnp_device"].as_string());
    WriteADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_DISKS", hwid["disks"].as_string());
}

jsoncons::json CHWID::LoadHWIDCaches()
{
    jsoncons::json json;
    json["cpu"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_CPU");
    json["motherboard_serial"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_MTS");
    json["bios"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_ProgramData), "ATOMICSHIELD_BIOS");
    json["pnp_device"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_PNPDEV");
    json["disks"] = ReadADS(SharedUtil::GetKnownDirectory(FOLDERID_RoamingAppData), "ATOMICSHIELD_DISKS");
    
    return json;
}

bool CHWID::WriteADS(std::string strPath, std::string strStreamName, std::string strData)
{
    if (strPath.empty())
        return false;

    if (strPath.back() == '\\')
        strPath.pop_back();

    std::string strADSPath = strPath + ":" + strStreamName;
    std::ofstream ads(strADSPath, std::ios::out | std::ios::trunc);
    if (!ads)
    {
        SharedUtil::AddDebugLog("Unable to write to ads %s", strADSPath.c_str());
        return false;
    }

    ads << strData;
    ads.close();
    return true;
}

std::string CHWID::ReadADS(std::string strPath, std::string strStreamName)
{
    if (strPath.empty())
        return "";

    if (strPath.back() == '\\')
        strPath.pop_back();

    std::string strADSPath = strPath + ":" + strStreamName;
    std::ifstream ads(strADSPath, std::ios::binary);
    if (!ads)
    {
        SharedUtil::AddDebugLog("Unable to read from ads %s", strADSPath.c_str());
        return "";
    }
    std::string data((std::istreambuf_iterator<char>(ads)), std::istreambuf_iterator<char>());

    ads.close();
    return data;
}