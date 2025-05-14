#include "StdInc.h"



bool FileAuthentication::IsFileSigned(const std::string& filePath)
{
    int          size_needed = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0);
    std::wstring wFilePath(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, &wFilePath[0], size_needed);

    LONG               status;
    WINTRUST_FILE_INFO fileInfo = {0};
    WINTRUST_DATA      winTrustData = {0};

    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = wFilePath.c_str();
    fileInfo.hFile = NULL;
    fileInfo.pgKnownSubject = NULL;

    winTrustData.cbStruct = sizeof(WINTRUST_DATA);
    winTrustData.pPolicyCallbackData = NULL;
    winTrustData.pSIPClientData = NULL;
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.dwStateAction = WTD_STATEACTION_IGNORE;
    winTrustData.hWVTStateData = NULL;
    winTrustData.pFile = &fileInfo;
    winTrustData.dwProvFlags = WTD_SAFER_FLAG;

    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    status = WinVerifyTrust(NULL, &policyGUID, &winTrustData);

    return status == ERROR_SUCCESS;
}










bool FileAuthentication::VerifyEmbeddedSignature(LPCWSTR filePath)
{
    WINTRUST_FILE_INFO fileData;
    WINTRUST_DATA      winTrustData;
    GUID               actionGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    memset(&fileData, 0, sizeof(fileData));
    fileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileData.pcwszFilePath = filePath;

    memset(&winTrustData, 0, sizeof(winTrustData));
    winTrustData.cbStruct = sizeof(WINTRUST_DATA);
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.pFile = &fileData;
    winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;

    LONG status = WinVerifyTrust(NULL, &actionGUID, &winTrustData);

    if (status != ERROR_SUCCESS)
    {
        if (status == TRUST_E_NOSIGNATURE || status == TRUST_E_BAD_DIGEST)
            return FALSE;

        if (status == CERT_E_REVOKED || status == CERT_E_EXPIRED || status == CERT_E_UNTRUSTEDROOT || status == CERT_E_CHAINING)
        {
            SharedUtil::AddDebugLog("Revoked signature detected");            // todo: flag
            return FALSE;
        }
    }

    /* success, cleanup */
    winTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &actionGUID, &winTrustData);

    return TRUE;
}

bool FileAuthentication::VerifyCatalogSignature(LPCWSTR filePath)
{
    HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    HCATADMIN hCatAdmin = NULL;
    if (!CryptCATAdminAcquireContext(&hCatAdmin, NULL, 0))
    {
        CloseHandle(hFile);
        return false;
    }

    BYTE  pbHash[100];
    DWORD cbHash = sizeof(pbHash);
    if (!CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, pbHash, 0))
    {
        CryptCATAdminReleaseContext(hCatAdmin, 0);
        CloseHandle(hFile);
        return false;
    }

    CATALOG_INFO CatInfo;
    memset(&CatInfo, 0, sizeof(CATALOG_INFO));
    CatInfo.cbStruct = sizeof(CATALOG_INFO);

    HCATINFO hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, pbHash, cbHash, 0, NULL);
    if (hCatInfo == NULL)
    {
        CryptCATAdminReleaseContext(hCatAdmin, 0);
        CloseHandle(hFile);
        return false;
    }

    if (!CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0))
    {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        CryptCATAdminReleaseContext(hCatAdmin, 0);
        CloseHandle(hFile);
        return false;
    }

    WINTRUST_CATALOG_INFO WinTrustCatalogInfo;
    memset(&WinTrustCatalogInfo, 0, sizeof(WinTrustCatalogInfo));
    WinTrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;
    WinTrustCatalogInfo.pcwszMemberTag = NULL;
    WinTrustCatalogInfo.pcwszMemberFilePath = filePath;
    WinTrustCatalogInfo.hMemberFile = hFile;
    WinTrustCatalogInfo.pbCalculatedFileHash = pbHash;
    WinTrustCatalogInfo.cbCalculatedFileHash = cbHash;

    GUID          ActionGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;
    memset(&WinTrustData, 0, sizeof(WinTrustData));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.pPolicyCallbackData = NULL;
    WinTrustData.pSIPClientData = NULL;
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    WinTrustData.hWVTStateData = NULL;
    WinTrustData.pwszURLReference = NULL;
    WinTrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    WinTrustData.dwUIContext = 0;
    WinTrustData.pCatalog = &WinTrustCatalogInfo;

    LONG lStatus = WinVerifyTrust(NULL, &ActionGuid, &WinTrustData);

    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &ActionGuid, &WinTrustData);

    CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
    CryptCATAdminReleaseContext(hCatAdmin, 0);
    CloseHandle(hFile);

    return lStatus == ERROR_SUCCESS;
}

bool FileAuthentication::HasSignature(LPCWSTR filePath)
{
    return (FileAuthentication::VerifyEmbeddedSignature(filePath) || FileAuthentication::VerifyCatalogSignature(filePath));
}