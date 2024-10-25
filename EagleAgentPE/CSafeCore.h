#pragma once
#include <Windows.h>
#include <openssl/aes.h>
#include "openssl/pem.h"
#include "openssl/err.h"

#include <string>
#include <vector>

class CSafeCore
{
public:
    CSafeCore();
    ~CSafeCore();

    BYTE* Encrypt(const BYTE* buffer);
    std::string       Decrypt(const BYTE* buffer);

private:
    std::vector<std::vector<BYTE>> m_vAESKeys = {{0XA,  0XB1, 0X30, 0X81, 0XE0, 0XE7, 0X44, 0X61, 0XA3, 0X9C, 0XE,  0X2C, 0X64, 0XA2, 0XBB, 0XCE,
                                                  0XBE, 0XD3, 0X1E, 0X79, 0XA,  0XFF, 0X14, 0X5C, 0X94, 0X61, 0X8A, 0XE2, 0XF8, 0XFC, 0X72, 0XD0},
                                                 {0X3A, 0X34, 0XC5, 0X5E, 0XBE, 0XED, 0XA3, 0X9D, 0X2B, 0X12, 0XBF, 0XF,  0X20, 0X22, 0X55, 0X1D,
                                                  0X85, 0XDE, 0XD3, 0X1E, 0X94, 0XA6, 0XC3, 0X1,  0X3F, 0XC5, 0X4A, 0X51, 0XD8, 0X8,  0X32, 0X60},
                                                 {0XF,  0X16, 0XC0, 0X75, 0XDC, 0X23, 0X27, 0XD6, 0X99, 0X30, 0XB2, 0X25, 0XAD, 0X11, 0X70, 0XCC,
                                                  0X69, 0X16, 0XF4, 0XFA, 0X39, 0X43, 0XE1, 0XFC, 0X40, 0XEE, 0XEC, 0X89, 0X6E, 0XB1, 0XDE, 0XB9},
                                                 {0X6B, 0XDA, 0X12, 0X62, 0X6,  0X7F, 0X8F, 0X39, 0X9F, 0X39, 0X70, 0X5B, 0X2C, 0X2D, 0XFF, 0XBF,
                                                  0X44, 0X66, 0X1F, 0XA5, 0X7B, 0X7B, 0X44, 0X8E, 0XF7, 0X3A, 0XB6, 0X41, 0X61, 0X8A, 0XB2, 0X2},
                                                 {0X47, 0X9E, 0XA2, 0X33, 0XCF, 0X1,  0XBF, 0X28, 0XF1, 0X6A, 0X1E, 0XB5, 0X2B, 0X1B, 0X18, 0X6D,
                                                  0X36, 0X2D, 0X28, 0XFF, 0XAE, 0X25, 0X2D, 0X6C, 0XB3, 0XB7, 0XB2, 0XCA, 0XD3, 0X29, 0X97, 0XC6},
                                                 {0XEB, 0X32, 0XCD, 0XAF, 0X90, 0XDE, 0XE6, 0X6A, 0X74, 0XAC, 0X97, 0X7E, 0XFF, 0X30, 0XCB, 0X52,
                                                  0X9D, 0XAE, 0X78, 0XA,  0X64, 0X35, 0XB3, 0X95, 0X56, 0X9C, 0X8,  0X6D, 0X74, 0X1B, 0X98, 0X5E},
                                                 {0X83, 0XAA, 0X4D, 0X60, 0XFB, 0X97, 0XBB, 0X8D, 0XF7, 0X6B, 0X9B, 0XBF, 0X17, 0X8C, 0X90, 0X63,
                                                  0X8A, 0XB3, 0X67, 0X3,  0XF3, 0X19, 0X4F, 0X28, 0X8F, 0X68, 0X4,  0XC2, 0X6A, 0XA3, 0X12, 0X55},
                                                 {0X7E, 0X85, 0X47, 0X4,  0X2D, 0X58, 0X19, 0XBC, 0X97, 0X2,  0X2F, 0X39, 0X71, 0XF9, 0X9F, 0XD,
                                                  0X7A, 0XD2, 0XED, 0XAB, 0XA7, 0XCA, 0XEF, 0X17, 0X87, 0X2A, 0X12, 0X34, 0X25, 0X46, 0X37, 0XF0}};

    std::vector<std::vector<BYTE>> m_vAESIVs = {{0X16, 0X2E, 0X60, 0X94, 0XC5, 0X41, 0XB, 0X7E, 0X23, 0XD8, 0X7B, 0X94, 0X78, 0XA1, 0X58, 0X78},
                                                {0XCC, 0XD0, 0X36, 0X12, 0X4D, 0XB6, 0X17, 0X18, 0X64, 0X9C, 0XDE, 0XDC, 0XD, 0X3D, 0X41, 0X3},
                                                {0X5B, 0XF6, 0XB3, 0XFC, 0XFC, 0XEF, 0XAE, 0X4, 0XA, 0XDF, 0X88, 0X62, 0XA3, 0XAC, 0X4C, 0X99},
                                                {0XA, 0XB9, 0X0, 0XFD, 0X50, 0X1A, 0X3E, 0XC6, 0X8D, 0X12, 0X22, 0XC9, 0X5C, 0X20, 0X11, 0XDF},
                                                {0X47, 0XD3, 0X2E, 0XAE, 0X72, 0XB, 0X96, 0X2C, 0X6C, 0XF6, 0X14, 0X4C, 0X18, 0XDE, 0XB0, 0X83},
                                                {0X8F, 0XC4, 0X3E, 0X4D, 0XBB, 0XEA, 0XE6, 0XD1, 0X14, 0XDE, 0XE9, 0XC0, 0X44, 0X8, 0X5A, 0XA4},
                                                {0X9E, 0XB3, 0X86, 0XA3, 0XE7, 0X7C, 0X8B, 0X17, 0X56, 0XFC, 0XBA, 0X17, 0XFC, 0XC, 0X6C, 0X22},
                                                {0XB3, 0X34, 0XAA, 0XD2, 0X15, 0X5C, 0XA, 0XBE, 0X56, 0X2C, 0XE8, 0XD7, 0X98, 0X61, 0X1E, 0X84}};
};

extern CSafeCore* g_pSafeCore;