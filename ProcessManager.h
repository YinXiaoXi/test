#pragma once

#include "Common.h"

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();
    
    // 禁用拷贝构造和赋值
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    
    // 进程查询功能
    std::vector<ProcessInfo> GetProcessList();
    std::vector<ProcessInfo> FindProcesses(const WString& processName);
    ProcessInfo FindProcess(DWORD processId);
    ProcessInfo FindProcess(const WString& processName);
    
    // 进程控制功能
    bool SuspendProcess(const WString& processName);
    bool ResumeProcess(const WString& processName);
    bool SuspendProcess(DWORD processId);
    bool ResumeProcess(DWORD processId);
    bool TerminateProcess(const WString& processName, UINT exitCode = 0);
    bool TerminateProcess(DWORD processId, UINT exitCode = 0);
    
    // 进程信息查询
    bool IsProcessRunning(const WString& processName);
    bool IsProcessRunning(DWORD processId);
    DWORD GetProcessId(const WString& processName);
    std::vector<DWORD> GetProcessIds(const WString& processName);
    
    // 错误处理
    ErrorCode GetLastError() const { return m_lastError; }
    String GetLastErrorString() const;
    
private:
    ErrorCode m_lastError;
    
    void SetLastError(ErrorCode error);
    void SetLastError(DWORD win32Error);
    bool SuspendResumeThreads(DWORD processId, bool suspend);
    bool IsValidProcess(const ProcessInfo& process);
};