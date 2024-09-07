#include <string>
#include <algorithm>
#include "SharedUtil.h"


bool SharedUtil::TerminateProcess(DWORD dwPID)
{
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	bool  bInheritHandle = FALSE;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwPID);
	if (hProcess == NULL)
		return FALSE;

	bool result = ::TerminateProcess(hProcess, 0);

	CloseHandle(hProcess);

	return result;
}

bool SharedUtil::FindStringIC(const std::string& strHaystack, const std::string& strNeedle)
{
	auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(),
		[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
	return (it != strHaystack.end());
}