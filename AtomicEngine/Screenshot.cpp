#include "StdInc.h"
#include <GdiPlus.h>
#include <fstream>


using namespace Gdiplus;

int Screenshot::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    ImageCodecInfo* pImageCodecInfo = NULL;

    UINT uiNum, uiSize = 0;
    GetImageEncodersSize(&uiNum, &uiSize);
    if (uiSize == 0)
        return -1;            // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(uiSize));
    if (pImageCodecInfo == NULL)
        return -2;            // Failure

    GetImageEncoders(uiNum, uiSize, pImageCodecInfo);

    for (UINT j = 0; j < uiNum; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;            // Success
        }
    }

    free(pImageCodecInfo);
    return -3;            // Failure
}

bool Screenshot::BitmapToJpg(const std::wstring& wszFileName, HBITMAP hbmpImage, int iWidth, int iHeight, DWORD dwQuality)
{
    // TODO: Convert from memory
    auto pBmpData = Gdiplus::Bitmap::FromHBITMAP(hbmpImage, NULL);
    if (!pBmpData)
        return false;

    std::string  szFormat = "image/jpeg";
    std::wstring wszFormat(szFormat.begin(), szFormat.end());

    EncoderParameters encoderParameters;
    encoderParameters.Count = 1;
    encoderParameters.Parameter[0].Guid = EncoderQuality;
    encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
    encoderParameters.Parameter[0].NumberOfValues = 1;
    encoderParameters.Parameter[0].Value = &dwQuality;

    CLSID jpgClsid;
    int   result = Screenshot::GetEncoderClsid(wszFormat.c_str(), &jpgClsid);
    if (result >= 0)
    {
        pBmpData->Save(wszFileName.c_str(), &jpgClsid, &encoderParameters);
        delete pBmpData;
        return true;
    }

    SharedUtil::AddDebugLog("BitmapToJpg: Encoding failed!");
    delete pBmpData;
    return false;
}

bool Screenshot::CreateScreenshotEx(std::string* pszData, char* szError)
{
    static std::recursive_mutex            mutex;
    std::unique_lock<std::recursive_mutex> mutex_lock(mutex);

    /// GDI+ Init
    ULONG_PTR           gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;
    auto                sGDI = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    if (sGDI != Status::Ok)
    {
        sprintf(szError, "GdiplusStartup fail! Error: %u", (int)sGDI);
        SharedUtil::AddDebugLog(szError);
        return false;
    }

    /// Screen configs
    RECT rcDesktop;
    HWND hwDesktop = GetDesktopWindow();
    if (GetWindowRect(hwDesktop, &rcDesktop) == FALSE)
    {
        sprintf(szError, "GetWindowRect fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    int iWidth = rcDesktop.right;
    int iHeight = rcDesktop.bottom;

    /// Create screenshot
    auto hDCScreen = GetDC(NULL);
    if (hDCScreen == NULL)
    {
        sprintf(szError, "hDCScreen fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    auto hDC = CreateCompatibleDC(hDCScreen);
    if (hDC == NULL)
    {
        sprintf(szError, "hDC fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    auto hBitmap = CreateCompatibleBitmap(hDCScreen, iWidth, iHeight);
    if (hBitmap == NULL)
    {
        sprintf(szError, "hBitmap fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    auto hGdiObj = SelectObject(hDC, hBitmap);
    if (BitBlt(hDC, 0, 0, iWidth, iHeight, hDCScreen, 0, 0, SRCCOPY) == FALSE)
    {
        sprintf(szError, "BitBlt fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    auto         szTmpFileName = (std::filesystem::temp_directory_path() / SharedUtil::GenerateRandomString(8)).string();
    std::wstring wszName(szTmpFileName.begin(), szTmpFileName.end());
    if (!Screenshot::BitmapToJpg(wszName, hBitmap, iWidth, iHeight, 120))
    {
        sprintf(szError, "BitmapToJpg fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    /// Copy screenshot to Memory
    std::string szOutput = "";
    //auto        bF2mRet = CFileFunctions::File2Mem(szTmpFileName, &szOutput);
    std::ifstream file(szTmpFileName, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return false;            // File could not be opened
    }

    std::streamsize size = file.tellg();
    if (size < 0)
    {
        return false;            // Invalid file size
    }

    file.seekg(0, std::ios::beg);

    szOutput.resize(static_cast<size_t>(size));
    if (!file.read(&(szOutput)[0], size))
    {
        return false;            // Failed to read the file
    }

    if (pszData)
        *pszData = szOutput;

    /// Finalize
    DeleteFileA(szTmpFileName.c_str());

    /// Deinit GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return true;
}

bool Screenshot::CreateScreenshot(std::string* pszData, char* szError)
{
    auto bRet = false;
    __try
    {
        bRet = CreateScreenshotEx(pszData, szError);
    }
    __except (1)
    {
    }
    return bRet;
}