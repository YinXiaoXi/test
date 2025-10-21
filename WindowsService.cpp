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

    // ��ʼ������״̬
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
    // ��ʱ����ԭʼ����
    std::string originalCommand = m_Command;

    // ���������ִ��
    m_Command = command;
    bool result = HandleCommand();

    // �ָ�ԭʼ����
    m_Command = originalCommand;

    return result;
}

int WindowsService::Run() {
    // ���Դ���������ȷ����ʵ��
    SingleInstance singleInstance("Global\\WinlogonManagerService");

    if (!singleInstance.IsFirstInstance()) {
        std::cout << "��һ��ʵ���������У�������ݸ���..." << std::endl;

        // �����ǰ������ͷ��͸������е�ʵ��
        if (!m_Command.empty()) {
            IPCManager ipc;
            if (ipc.SendCommandToServer(m_Command)) {
                std::cout << "�����ѳɹ����͵������е�ʵ��" << std::endl;
                return 0;
            }
            else {
                std::cout << "�޷����ӵ������е�ʵ�������ܷ���δ����" << std::endl;
                // ����޷����ӣ����ܷ���û�����У����Ǽ���ִ��
                std::cout << "��ֱ��ִ������..." << std::endl;
                return 1;
            }
        }
        else {
            std::cout << "��������Ҫ����" << std::endl;
            return 0;
        }
    }

    // ����ǵ�һ��ʵ��������IPC������
    // ʹ�� [this] ����ǰʵ��
    if (!m_IPCManager.StartServer([this](const std::string& cmd) {
        return this->HandleIPCCommand(cmd);
        })) {
        std::cout << "����IPC������ʧ��" << std::endl;
        return 1;
    }
    std::cout << "IPC���������������ȴ�����..." << std::endl;

    // ԭ�еķ����߼����ֲ���
    if (!m_Command.empty()) {
        return HandleCommand() ? 0 : 1;
    }

    // ���û�в�����������״̬
    ServiceManager serviceManager(SERVICE_NAME);
    if (!serviceManager.IsInstalled()) {
        std::cout << "����δ��װ�����ڰ�װ..." << std::endl;
        if (!InstallService()) {
            std::cout << "��װ����ʧ��" << std::endl;
            return 1;
        }
        std::cout << "����װ�ɹ�����������..." << std::endl;
        if (!StartService()) {
            std::cout << "��������ʧ��" << std::endl;
            return 1;
        }
        return 0;
    }

    // ��������Ѱ�װ����Ϊ��������
    std::cout << "�Է���ģʽ����..." << std::endl;

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(serviceTable)) {
        std::cout << "����������Ʒַ���ʧ��: " << GetLastError() << std::endl;
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

    // �ڷ�������ʱ����IPC������
    // ����ֱ��ʹ�� s_pThis����Ϊ�����ھ�̬������
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

    // �ڷ���ֹͣʱֹͣIPC������
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
    // �����������߳�
    while (WaitForSingleObject(m_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        // ���������ӷ���Ķ�������
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
        std::cout << "δ֪����: " << m_Command << std::endl;
        std::cout << "��������: --install, --uninstall, --start, --stop, --suspend, --resume, --status" << std::endl;
        return false;
    }
}

bool WindowsService::InstallService() {
    ServiceManager serviceManager(SERVICE_NAME);

    wchar_t modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);

    return serviceManager.InstallService(
        L"Winlogon Manager Service",
        L"����winlogon���̵ķ���",
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
        std::cout << "winlogon.exe �ѹ���" << std::endl;
        return true;
    }
    std::cout << "���� winlogon.exe ʧ��" << std::endl;
    return false;
}

bool WindowsService::ResumeWinlogon() {
    ProcessManager processManager;
    if (processManager.ResumeProcess(L"winlogon.exe")) {
        m_WinlogonSuspended = false;
        std::cout << "winlogon.exe �ѻָ�" << std::endl;
        return true;
    }
    std::cout << "�ָ� winlogon.exe ʧ��" << std::endl;
    return false;
}

int main(int argc, char* argv[]) {
    WindowsService service;

    if (!service.ParseCommandLine(argc, argv)) {
        std::cout << "δ�ṩ������ʹ��Ĭ����Ϊ..." << std::endl;
    }

    return service.Run();
}
