#pragma once

#include "Common.h"
#include "ServiceManager.h"
#include "ProcessManager.h"
#include "IPCManager.h"

class WinlogonService {
public:
    WinlogonService();
    ~WinlogonService();
    
    // 禁用拷贝构造和赋值
    WinlogonService(const WinlogonService&) = delete;
    WinlogonService& operator=(const WinlogonService&) = delete;
    
    // 主要功能
    int Run(int argc, char* argv[]);
    bool ParseCommandLine(int argc, char* argv[]);
    
    // 命令处理
    bool HandleCommand(const String& command);
    void ShowHelp();
    
    // 服务管理命令
    bool InstallService();
    bool UninstallService();
    bool StartService();
    bool StopService();
    bool RestartService();
    bool QueryServiceStatus();
    
    // Winlogon进程管理命令
    bool SuspendWinlogon();
    bool ResumeWinlogon();
    bool QueryWinlogonStatus();
    
    // 错误处理
    ErrorCode GetLastError() const { return m_lastError; }
    String GetLastErrorString() const;
    
private:
    static WinlogonService* s_instance;
    static std::mutex s_instanceMutex;
    
    // 服务相关
    SERVICE_STATUS m_serviceStatus;
    SERVICE_STATUS_HANDLE m_serviceStatusHandle;
    HANDLE m_serviceStopEvent;
    std::atomic<bool> m_isRunningAsService;
    std::atomic<bool> m_winlogonSuspended;
    
    // 管理器
    std::unique_ptr<ServiceManager> m_serviceManager;
    std::unique_ptr<ProcessManager> m_processManager;
    std::unique_ptr<IPCManager> m_ipcManager;
    
    // 命令和错误
    String m_currentCommand;
    ErrorCode m_lastError;
    
    // 静态服务函数
    static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);
    
    // 内部方法
    void InitializeService();
    void CleanupService();
    void ServiceWorkerThread();
    bool HandleIPCCommand(const String& command);
    void SetLastError(ErrorCode error);
    void SetLastError(DWORD win32Error);
    void UpdateServiceStatus(DWORD currentState, DWORD waitHint = 0);
};