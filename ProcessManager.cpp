#include "ProcessManager.h"
#include <tlhelp32.h>
#include <psapi.h>

ProcessManager::ProcessManager() : m_lastError(ErrorCode::Success) {
}

ProcessManager::~ProcessManager() {
}

std::vector<ProcessInfo> ProcessManager::GetProcessList() {
    std::vector<ProcessInfo> processes;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        Logger::Log(LogLevel::Error, "Failed to create process snapshot, error: %s", Utils::GetLastErrorString().c_str());
        SetLastError(GetLastError());
        return processes;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            ProcessInfo info;
            info.processId = pe32.th32ProcessID;
            info.processName = pe32.szExeFile;
            info.threadCount = pe32.cntThreads;
            info.parentProcessId = pe32.th32ParentProcessID;
            
            if (IsValidProcess(info)) {
                processes.push_back(info);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    Logger::Log(LogLevel::Debug, "Found %zu processes", processes.size());
    return processes;
}

std::vector<ProcessInfo> ProcessManager::FindProcesses(const WString& processName) {
    std::vector<ProcessInfo> result;
    auto processes = GetProcessList();
    
    for (const auto& process : processes) {
        if (_wcsicmp(process.processName.c_str(), processName.c_str()) == 0) {
            result.push_back(process);
        }
    }
    
    Logger::Log(LogLevel::Debug, "Found %zu processes with name: %s", 
                result.size(), Utils::WStringToString(processName).c_str());
    return result;
}

ProcessInfo ProcessManager::FindProcess(DWORD processId) {
    ProcessInfo info;
    auto processes = GetProcessList();
    
    for (const auto& process : processes) {
        if (process.processId == processId) {
            return process;
        }
    }
    
    SetLastError(ErrorCode::ProcessNotFound);
    return info;
}

ProcessInfo ProcessManager::FindProcess(const WString& processName) {
    ProcessInfo info;
    auto processes = FindProcesses(processName);
    
    if (!processes.empty()) {
        return processes[0];
    }
    
    SetLastError(ErrorCode::ProcessNotFound);
    return info;
}

bool ProcessManager::SuspendProcess(const WString& processName) {
    auto processes = FindProcesses(processName);
    if (processes.empty()) {
        Logger::Log(LogLevel::Warning, "Process not found: %s", Utils::WStringToString(processName).c_str());
        SetLastError(ErrorCode::ProcessNotFound);
        return false;
    }
    
    bool anySuspended = false;
    for (const auto& process : processes) {
        if (SuspendProcess(process.processId)) {
            anySuspended = true;
        }
    }
    
    return anySuspended;
}

bool ProcessManager::ResumeProcess(const WString& processName) {
    auto processes = FindProcesses(processName);
    if (processes.empty()) {
        Logger::Log(LogLevel::Warning, "Process not found: %s", Utils::WStringToString(processName).c_str());
        SetLastError(ErrorCode::ProcessNotFound);
        return false;
    }
    
    bool anyResumed = false;
    for (const auto& process : processes) {
        if (ResumeProcess(process.processId)) {
            anyResumed = true;
        }
    }
    
    return anyResumed;
}

bool ProcessManager::SuspendProcess(DWORD processId) {
    if (SuspendResumeThreads(processId, true)) {
        Logger::Log(LogLevel::Info, "Process suspended successfully (PID: %d)", processId);
        return true;
    }
    
    Logger::Log(LogLevel::Error, "Failed to suspend process (PID: %d), error: %s", 
                processId, Utils::GetLastErrorString().c_str());
    return false;
}

bool ProcessManager::ResumeProcess(DWORD processId) {
    if (SuspendResumeThreads(processId, false)) {
        Logger::Log(LogLevel::Info, "Process resumed successfully (PID: %d)", processId);
        return true;
    }
    
    Logger::Log(LogLevel::Error, "Failed to resume process (PID: %d), error: %s", 
                processId, Utils::GetLastErrorString().c_str());
    return false;
}

bool ProcessManager::TerminateProcess(const WString& processName, UINT exitCode) {
    auto processes = FindProcesses(processName);
    if (processes.empty()) {
        Logger::Log(LogLevel::Warning, "Process not found: %s", Utils::WStringToString(processName).c_str());
        SetLastError(ErrorCode::ProcessNotFound);
        return false;
    }
    
    bool anyTerminated = false;
    for (const auto& process : processes) {
        if (TerminateProcess(process.processId, exitCode)) {
            anyTerminated = true;
        }
    }
    
    return anyTerminated;
}

bool ProcessManager::TerminateProcess(DWORD processId, UINT exitCode) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (!hProcess) {
        Logger::Log(LogLevel::Error, "Failed to open process (PID: %d), error: %s", 
                    processId, Utils::GetLastErrorString().c_str());
        SetLastError(GetLastError());
        return false;
    }
    
    BOOL result = ::TerminateProcess(hProcess, exitCode);
    CloseHandle(hProcess);
    
    if (result) {
        Logger::Log(LogLevel::Info, "Process terminated successfully (PID: %d)", processId);
        return true;
    }
    
    Logger::Log(LogLevel::Error, "Failed to terminate process (PID: %d), error: %s", 
                processId, Utils::GetLastErrorString().c_str());
    SetLastError(GetLastError());
    return false;
}

bool ProcessManager::IsProcessRunning(const WString& processName) {
    return !FindProcesses(processName).empty();
}

bool ProcessManager::IsProcessRunning(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess) {
        return false;
    }
    
    DWORD exitCode;
    BOOL result = GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    
    return result && exitCode == STILL_ACTIVE;
}

DWORD ProcessManager::GetProcessId(const WString& processName) {
    auto process = FindProcess(processName);
    return process.processId;
}

std::vector<DWORD> ProcessManager::GetProcessIds(const WString& processName) {
    std::vector<DWORD> pids;
    auto processes = FindProcesses(processName);
    
    for (const auto& process : processes) {
        pids.push_back(process.processId);
    }
    
    return pids;
}

String ProcessManager::GetLastErrorString() const {
    switch (m_lastError) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::InvalidParameter: return "Invalid parameter";
        case ErrorCode::AccessDenied: return "Access denied";
        case ErrorCode::ProcessNotFound: return "Process not found";
        case ErrorCode::IPCConnectionFailed: return "IPC connection failed";
        case ErrorCode::UnknownError: return "Unknown error";
        default: return "Unknown error";
    }
}

void ProcessManager::SetLastError(ErrorCode error) {
    m_lastError = error;
}

void ProcessManager::SetLastError(DWORD win32Error) {
    switch (win32Error) {
        case ERROR_ACCESS_DENIED: m_lastError = ErrorCode::AccessDenied; break;
        case ERROR_INVALID_PARAMETER: m_lastError = ErrorCode::InvalidParameter; break;
        case ERROR_PROCESS_NOT_FOUND: m_lastError = ErrorCode::ProcessNotFound; break;
        default: m_lastError = ErrorCode::UnknownError; break;
    }
}

bool ProcessManager::SuspendResumeThreads(DWORD processId, bool suspend) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        Logger::Log(LogLevel::Error, "Failed to create thread snapshot, error: %s", Utils::GetLastErrorString().c_str());
        SetLastError(GetLastError());
        return false;
    }
    
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    
    bool success = false;
    if (Thread32First(hSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == processId) {
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

bool ProcessManager::IsValidProcess(const ProcessInfo& process) {
    // 过滤掉系统进程和无效进程
    if (process.processId == 0 || process.processId == 4) { // System Idle Process, System
        return false;
    }
    
    // 检查进程名是否有效
    if (process.processName.empty()) {
        return false;
    }
    
    return true;
}