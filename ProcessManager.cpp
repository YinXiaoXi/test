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
		std::wcout << L"创建线程快照失败，错误代码: " << GetLastError() << std::endl;
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
		std::cout << "未找到进程: " << std::endl;
		return false;
	}

	bool anySuspended = false;
	for (DWORD pid : pids) {
		if (SuspendResumeThreads(pid, true)) {
			std::cout << "已挂起进程 (PID: " << pid << ")" << std::endl;
			anySuspended = true;
		} else {
			std::cout << "挂起进程失败 (PID: " << pid << ")，错误代码: " << GetLastError() << std::endl;
		}
	}

	return anySuspended;
}

bool ProcessManager::ResumeProcess(const wchar_t* processName) {
	auto pids = FindProcesses(processName);
	if (pids.empty()) {
		std::cout << "未找到进程: " << std::endl;
		return false;
	}

	bool anyResumed = false;
	for (DWORD pid : pids) {
		if (SuspendResumeThreads(pid, false)) {
			std::cout << "已恢复进程 (PID: " << pid << ")" << std::endl;
			anyResumed = true;
		} else {
			std::cout << "恢复进程失败 (PID: " << pid << ")，错误代码: " << GetLastError() << std::endl;
		}
	}

	return anyResumed;
}