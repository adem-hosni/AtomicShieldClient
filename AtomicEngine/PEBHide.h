#pragma once
#include "StdInc.h"


#define UNLINK(x) \
    (x).Flink->Blink = (x).Blink; \
    (x).Blink->Flink = (x).Flink;

typedef struct _LDRDATA_TABLE_ENTRY
{
    LIST_ENTRY     InLoadOrderLinks;
    LIST_ENTRY     InMemoryOrderLinks;
    LIST_ENTRY     InInitializationOrderLinks;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG          Flags;
    USHORT         LoadCount;
    USHORT         TlsIndex;
    LIST_ENTRY     HashLinks;
    ULONG          TimeDateStamp;
} LDRDATA_TABLE_ENTRY, *PLDRDATA_TABLE_ENTRY;

typedef struct _PEB_LDRDATA
{
    ULONG      Length;
    UCHAR      Initialized;
    PVOID      SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDRDATA, *PPEB_LDRDATA;


namespace PEBHide
{
    void EraseSelfPEHeader(LPVOID lpBaseAddress);
    void UnlinkSelfLdrModule(LPVOID lpBaseAddress);
};
