#include "StdInc.h"
#include <GdiPlus.h>
#include <fstream>

using namespace Gdiplus;

namespace
{
    struct EnumData
    {
        DWORD pid;
        HWND  hwnd;
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        EnumData* data = reinterpret_cast<EnumData*>(lParam);
        DWORD     windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);

        // Filter only visible, top-level windows
        if (windowPid == data->pid && GetWindow(hwnd, GW_OWNER) == nullptr && IsWindowVisible(hwnd))
        {
            data->hwnd = hwnd;
            return FALSE;            // stop
        }
        return TRUE;            // continue
    }

    HWND GetMainWindowHandle(DWORD pid)
    {
        EnumData data{pid, nullptr};
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
        return data.hwnd;
    }
}


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

    /// Find FiveM window
    HWND hWndGame = GetMainWindowHandle(SharedUtil::GetFivemProcessID());

    if (!hWndGame)
    {
        sprintf(szError, "FiveM window not found!");
        SharedUtil::AddDebugLog(szError);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    /// Get game window rect
    RECT rcGame;
    if (!GetClientRect(hWndGame, &rcGame))
    {
        sprintf(szError, "GetClientRect fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    int iWidth = rcGame.right - rcGame.left;
    int iHeight = rcGame.bottom - rcGame.top;

    /// Get DC for game window
    HDC hDCScreen = GetDC(hWndGame);
    if (!hDCScreen)
    {
        sprintf(szError, "GetDC fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    HDC     hDC = CreateCompatibleDC(hDCScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hDCScreen, iWidth, iHeight);
    SelectObject(hDC, hBitmap);

    if (!BitBlt(hDC, 0, 0, iWidth, iHeight, hDCScreen, 0, 0, SRCCOPY))
    {
        sprintf(szError, "BitBlt fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        ReleaseDC(hWndGame, hDCScreen);
        DeleteDC(hDC);
        DeleteObject(hBitmap);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    /// Save to temp file
    auto         szTmpFileName = (std::filesystem::temp_directory_path() / SharedUtil::GenerateRandomString(8)).string();
    std::wstring wszName(szTmpFileName.begin(), szTmpFileName.end());

    if (!Screenshot::BitmapToJpg(wszName, hBitmap, iWidth, iHeight, 120))
    {
        sprintf(szError, "BitmapToJpg fail! Error: %u", GetLastError());
        SharedUtil::AddDebugLog(szError);

        ReleaseDC(hWndGame, hDCScreen);
        DeleteDC(hDC);
        DeleteObject(hBitmap);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    /// Copy to memory
    std::ifstream file(szTmpFileName, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string szOutput(size, '\0');
    if (!file.read(&szOutput[0], size))
    {
        return false;
    }

    if (pszData)
        *pszData = szOutput;

    DeleteFileA(szTmpFileName.c_str());

    /// Cleanup
    ReleaseDC(hWndGame, hDCScreen);
    DeleteDC(hDC);
    DeleteObject(hBitmap);

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