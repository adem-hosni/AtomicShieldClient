#include "SharedChecks.h"
#include "XorStr.h"
#include <vector>
#include <TlHelp32.h>
#include <string>
#include "SharedUtil.h"

BOOL CALLBACK WindowNameFilter(HWND hWnd, LPARAM lParam)
{
	char szWindowTitle[256];
	std::vector<const char*> vBlacklistWindowsNames = { XorStr("HTTP Debugger Pro").c_str(), XorStr("x32dbg").c_str(), XorStr("x64dbg").c_str(), XorStr("x96dbg").c_str(), XorStr("Cheat Engine").c_str(), XorStr("Wireshark").c_str(), XorStr("procmon").c_str(), XorStr("Process Monitor").c_str() };
	if (GetWindowTextA(hWnd, szWindowTitle, sizeof(szWindowTitle))) {
		std::string windowTitle(szWindowTitle);
		if (!windowTitle.empty()) {
			for (const auto& IterWindowName : vBlacklistWindowsNames)
			{
				if (SharedUtil::FindStringIC(windowTitle, IterWindowName))
				{
					char szBanReason[256];
					sprintf(szBanReason, XorStr("%s Detected").c_str(), windowTitle);

					reinterpret_cast<void(*)(char*)>(lParam)(szBanReason);

					PostMessage(hWnd, WM_CLOSE, 0, 0);
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

void SharedChecks::CheckProcessList(void(*found_process)(char* szProcessName))
{
	std::vector<std::string> vBlacklistProcesses =
	{ _("x96dbg"), XorStr("x64dbg"), XorStr("x32dbg"),
	  _("die"), XorStr("ida"), "ida64", XorStr("dnSpy"),
	  _("dnSpy32"), XorStr("cheatengine-x86_64-SSE4-AVX2"),
	  _("cheatengine-x86_64"), XorStr("cheatengine-i386"),
	  _("ProcessHacker"), XorStr("HTTPDebugger"), XorStr("HTTPDebuggerPro"),
	  _("wireshark"), XorStr("die") };

	while (true)
	{

		EnumWindows(WindowNameFilter, reinterpret_cast<LPARAM>(found_process));

		HANDLE hProcessSnap;
		PROCESSENTRY32 pe32;

		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE) {
			return;
		}

		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (!Process32First(hProcessSnap, &pe32)) {
			CloseHandle(hProcessSnap);
			return;
		}
		do {
			char* szProcessName = pe32.szExeFile;
			for (auto& szBlackListProcessName : vBlacklistProcesses)
			{
				char szBlackListedProcess[256];
				sprintf(szBlackListedProcess, XorStr("%s.exe").c_str(), szBlackListProcessName.c_str());

				if (strcmp(szBlackListedProcess, szProcessName) == 0)
				{
					char szBanReason[256];
					sprintf(szBanReason, XorStr("%s Detected").c_str(), szProcessName);
					found_process(szBanReason);
					DWORD dwTargetPID = SharedUtil::GetProcessID(szProcessName);
					SharedUtil::TerminateProcess(dwTargetPID);
					__fastfail(0);
				}
			}
		} while (Process32Next(hProcessSnap, &pe32));
		CloseHandle(hProcessSnap);
	}
}
