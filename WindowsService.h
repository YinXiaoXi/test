#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "IPCManager.h"

class WindowsService {
public:
    WindowsService();
    ~WindowsService();

    bool ParseCommandLine(int argc, char* argv[]);
    int Run();

private:
    static WindowsService* s_pThis;
    static std::mutex s_pThisMutex;

    // ������غ���
    static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);
    void ServiceWorkerThread();

    // ���������
    bool HandleCommand();
    bool HandleCommandString(const std::string& command);
    bool InstallService();
    bool UninstallService();
    bool StartService();
    bool StopService();
    bool QueryService();

    // ���̲�������
    bool SuspendWinlogon();
    bool ResumeWinlogon();

    // IPC������ص�
    bool HandleIPCCommand(const std::string& command);

    // ��Ա����
    SERVICE_STATUS m_ServiceStatus;
    SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
    HANDLE m_ServiceStopEvent;
    std::string m_Command;
    bool m_IsRunningAsService;
    HANDLE m_InstanceMutex;

    // IPC������
    IPCManager m_IPCManager;

    // ����״̬
    bool m_WinlogonSuspended;
};
