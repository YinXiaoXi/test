#include "ProcessManager.h"
#include <iostream>

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
		return false;
	}

	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);

	bool success = false;
	if (Thread32First(hSnapshot, &te32)) {
		do {
			if (te32.th32OwnerProcessID == pid) {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
				if (hThread) {
					if (suspend) {
						SuspendThread(hThread);
					} else {
						ResumeThread(hThread);
					}
					CloseHandle(hThread);
					success = true;
				}
			}
		} while (Thread32Next(hSnapshot, &te32));
	}

	CloseHandle(hSnapshot);
	return success;
}

bool ProcessManager::SuspendProcess(const wchar_t* processName) {
	auto pids = FindProcesses(processName);
	if (pids.empty()) {
		std::wcout << L"未找到进程: " << processName << std::endl;
		return false;
	}

	bool anySuspended = false;
	for (DWORD pid : pids) {
		if (SuspendResumeThreads(pid, true)) {
			std::wcout << L"已挂起进程 " << processName << L" (PID: " << pid << L")" << std::endl;
			anySuspended = true;
		}
	}

	return anySuspended;
}

bool ProcessManager::ResumeProcess(const wchar_t* processName) {
	auto pids = FindProcesses(processName);
	if (pids.empty()) {
		std::wcout << L"未找到进程: " << processName << std::endl;
		return false;
	}

	bool anyResumed = false;
	for (DWORD pid : pids) {
		if (SuspendResumeThreads(pid, false)) {
			std::wcout << L"已恢复进程 " << processName << L" (PID: " << pid << L")" << std::endl;
			anyResumed = true;
		}
	}

	return anyResumed;
}