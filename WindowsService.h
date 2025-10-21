#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include "IPCManager.h"

class WindowsService {
public:
    WindowsService();
    ~WindowsService();

    bool ParseCommandLine(int argc, char* argv[]);
    int Run();

private:
    static WindowsService* s_pThis;

    // 服务相关函数
    static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);
    void ServiceWorkerThread();

    // 命令处理函数
    bool HandleCommand();
    bool HandleCommandString(const std::string& command);
    bool InstallService();
    bool UninstallService();
    bool StartService();
    bool StopService();
    bool QueryService();

    // 进程操作函数
    bool SuspendWinlogon();
    bool ResumeWinlogon();

    // IPC命令处理回调
    bool HandleIPCCommand(const std::string& command);

    // 成员变量
    SERVICE_STATUS m_ServiceStatus;
    SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
    HANDLE m_ServiceStopEvent;
    std::string m_Command;
    bool m_IsRunningAsService;
    HANDLE m_InstanceMutex;

    // IPC管理器
    IPCManager m_IPCManager;

    // 进程状态
    bool m_WinlogonSuspended;
};
