#include "ProcessManager.h"
#include <iostream>
#include <tlhelp32.h>

ProcessManager::ProcessManager() {
}

ProcessManager::~ProcessManager() {
}

std::vector<ProcessInfo> ProcessManager::GetProcessList() {
	std::vector<ProcessInfo> processes;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return processes;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			ProcessInfo info;
			info.pid = pe32.th32ProcessID;
			info.name = pe32.szExeFile;
			processes.push_back(info);
		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return processes;
}

std::vector<DWORD> ProcessManager::FindProcesses(const wchar_t* processName) {
	std::vector<DWORD> pids;
	auto processes = GetProcessList();

	for (const auto& process : processes) {
		if (_wcsicmp(process.name.c_str(), processName) == 0) {
			pids.push_back(process.pid);
		}
	}

	return pids;
}

bool ProcessManager::SuspendResumeThreads(DWORD pid, bool suspend) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		std::wcout << L"