#include "WindowsService.h"
#include "ServiceManager.h"
#include "ProcessManager.h"
#include "SingleInstance.h"
#include <iostream>
#include <sstream>
#include "IPCManager.h"

#define SERVICE_NAME L"RayLinkService"

WindowsService* WindowsService::s_pThis = nullptr;

WindowsService::WindowsService()
    : m_ServiceStopEvent(INVALID_HANDLE_VALUE)
    , m_IsRunningAsService(false)
    , m_InstanceMutex(INVALID_HANDLE_VALUE)
    , m_WinlogonSuspended(false) {
    s_pThis = this;

    // 初始化服务状态
    m_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    m_ServiceStatus.dwServiceSpecificExitCode = 0;
    m_ServiceStatus.dwCheckPoint = 0;
    m_ServiceStatus.dwWaitHint = 0;
}

WindowsService::~WindowsService() {
    if (m_ServiceStopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_ServiceStopEvent);
    }
    if (m_InstanceMutex != INVALID_HANDLE_VALUE) {
        CloseHandle(m_InstanceMutex);
    }
}

bool WindowsService::ParseCommandLine(int argc, char* argv[]) {
    if (argc > 1) {
        m_Command = argv[1];
        return true;
    }
    return false;
}

bool WindowsService::HandleIPCCommand(const std::string& command) {
    return HandleCommandString(command);
}

bool WindowsService::HandleCommandString(const std::string& command) {
    // 临时保存原始命令
    std::string originalCommand = m_Command;

    // 设置新命令并执行
    m_Command = command;
    bool result = HandleCommand();

    // 恢复原始命令
    m_Command = originalCommand;

    return result;
}

int WindowsService::Run() {
    // 尝试创建互斥体确保单实例
    SingleInstance singleInstance("Global\\WinlogonManagerService");

    if (!singleInstance.IsFirstInstance()) {
        std::cout << "另一个实例正在运行，将命令传递给它..." << std::endl;

        // 如果当前有命令，就发送给已运行的实例
        if (!m_Command.empty()) {
            IPCManager ipc;
            if (ipc.SendCommandToServer(m_Command)) {
                std::cout << "命令已成功发送到运行中的实例" << std::endl;
                return 0;
            }
            else {
                std::cout << "无法连接到运行中的实例，可能服务未运行" << std::endl;
                // 如果无法连接，可能服务没有运行，我们继续执行
                std::cout << "将直接执行命令..." << std::endl;
                return 1;
            }
        }
        else {
            std::cout << "无命令需要传递" << std::endl;
            return 0;
        }
    }

    // 如果是第一个实例，启动IPC服务器
    // 使用 [this] 捕获当前实例
    if (!m_IPCManager.StartServer([this](const std::string& cmd) {
        return this->HandleIPCCommand(cmd);
        })) {
        std::cout << "启动IPC服务器失败" << std::endl;
        return 1;
    }
    std::cout << "IPC服务器已启动，等待命令..." << std::endl;

    // 原有的服务逻辑保持不变
    if (!m_Command.empty()) {
        return HandleCommand() ? 0 : 1;
    }

    // 如果没有参数，检查服务状态
    ServiceManager serviceManager(SERVICE_NAME);
    if (!serviceManager.IsInstalled()) {
        std::cout << "服务未安装，正在安装..." << std::endl;
        if (!InstallService()) {
            std::cout << "安装服务失败" << std::endl;
            return 1;
        }
        std::cout << "服务安装成功，正在启动..." << std::endl;
        if (!StartService()) {
            std::cout << "启动服务失败" << std::endl;
            return 1;
        }
        return 0;
    }

    // 如果服务已安装，作为服务运行
    std::cout << "以服务模式运行..." << std::endl;

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(serviceTable)) {
        std::cout << "启动服务控制分发器失败: " << GetLastError() << std::endl;
        return 1;
    }

    return 0;
}

void WINAPI WindowsService::ServiceMain(DWORD argc, LPTSTR* argv) {
    if (!s_pThis) return;

    s_pThis->m_ServiceStatusHandle = RegisterServiceCtrlHandler(
        SERVICE_NAME, ServiceCtrlHandler);

    if (!s_pThis->m_ServiceStatusHandle) {
        return;
    }

    s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    s_pThis->m_ServiceStatus.dwWaitHint = 3000;
    SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);

    s_pThis->m_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (s_pThis->m_ServiceStopEvent == NULL) {
        s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);
        return;
    }

    // 在服务启动时启动IPC服务器
    // 这里直接使用 s_pThis，因为我们在静态函数中
    s_pThis->m_IPCManager.StartServer([](const std::string& cmd) {
        if (WindowsService::s_pThis) {
            return WindowsService::s_pThis->HandleIPCCommand(cmd);
        }
        return false;
        });

    s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);

    s_pThis->m_IsRunningAsService = true;
    s_pThis->ServiceWorkerThread();

    WaitForSingleObject(s_pThis->m_ServiceStopEvent, INFINITE);

    // 在服务停止时停止IPC服务器
    s_pThis->m_IPCManager.StopServer();

    s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);
}

void WINAPI WindowsService::ServiceCtrlHandler(DWORD dwCtrl) {
    if (!s_pThis) return;

    switch (dwCtrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);
        SetEvent(s_pThis->m_ServiceStopEvent);
        break;
    default:
        break;
    }
}

void WindowsService::ServiceWorkerThread() {
    // 服务主工作线程
    while (WaitForSingleObject(m_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        // 这里可以添加服务的定期任务
        Sleep(1000);
    }
}

bool WindowsService::HandleCommand() {
    if (m_Command == "--install") {
        return InstallService();
    }
    else if (m_Command == "--uninstall") {
        return UninstallService();
    }
    else if (m_Command == "--start") {
        return StartService();
    }
    else if (m_Command == "--stop") {
        return StopService();
    }
    else if (m_Command == "--suspend") {
        return SuspendWinlogon();
    }
    else if (m_Command == "--resume") {
        return ResumeWinlogon();
    }
    else if (m_Command == "--status") {
        return QueryService();
    }
    else {
        std::cout << "未知命令: " << m_Command << std::endl;
        std::cout << "可用命令: --install, --uninstall, --start, --stop, --suspend, --resume, --status" << std::endl;
        return false;
    }
}

bool WindowsService::InstallService() {
    ServiceManager serviceManager(SERVICE_NAME);

    wchar_t modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);

    return serviceManager.InstallService(
        L"Winlogon Manager Service",
        L"管理winlogon进程的服务",
        modulePath
    );
}

bool WindowsService::UninstallService() {
    ServiceManager serviceManager(SERVICE_NAME);
    return serviceManager.UninstallService();
}

bool WindowsService::StartService() {
    ServiceManager serviceManager(SERVICE_NAME);
    return serviceManager.StartService();
}

bool WindowsService::StopService() {
    ServiceManager serviceManager(SERVICE_NAME);
    return serviceManager.StopService();
}

bool WindowsService::QueryService() {
    ServiceManager serviceManager(SERVICE_NAME);
    return serviceManager.QueryStatus();
}

bool WindowsService::SuspendWinlogon() {
    ProcessManager processManager;
    if (processManager.SuspendProcess(L"winlogon.exe")) {
        m_WinlogonSuspended = true;
        std::cout << "winlogon.exe 已挂起" << std::endl;
        return true;
    }
    std::cout << "挂起 winlogon.exe 失败" << std::endl;
    return false;
}

bool WindowsService::ResumeWinlogon() {
    ProcessManager processManager;
    if (processManager.ResumeProcess(L"winlogon.exe")) {
        m_WinlogonSuspended = false;
        std::cout << "winlogon.exe 已恢复" << std::endl;
        return true;
    }
    std::cout << "恢复 winlogon.exe 失败" << std::endl;
    return false;
}

int main(int argc, char* argv[]) {
    WindowsService service;

    if (!service.ParseCommandLine(argc, argv)) {
        std::cout << "未提供参数，使用默认行为..." << std::endl;
    }

    return service.Run();
}
