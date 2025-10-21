#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <tlhelp32.h>

struct ProcessInfo {
	DWORD pid;
	std::wstring name;
};

class ProcessManager {
public:
	ProcessManager();
	~ProcessManager();

	bool SuspendProcess(const wchar_t* processName);
	bool ResumeProcess(const wchar_t* processName);
	std::vector<DWORD> FindProcesses(const wchar_t* processName);

private:
	std::vector<ProcessInfo> GetProcessList();
	bool SuspendResumeThreads(DWORD pid, bool suspend);
};