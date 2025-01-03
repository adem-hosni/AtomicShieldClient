#pragma once
#include "StdInc.h"

namespace Screenshot
{
    int  GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
    bool BitmapToJpg(const std::wstring& wszFileName, HBITMAP hbmpImage, int iWidth, int iHeight, DWORD dwQuality);
    bool CreateScreenshotEx(std::string* pszData);
    bool CreateScreenshot(std::string* pszData);
};
