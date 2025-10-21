#include "WindowsService.h"
#include "ServiceManager.h"
#include "ProcessManager.h"
#include "SingleInstance.h"
#include <iostream>
#include <sstream>
#include "IPCManager.h"

#define SERVICE_NAME L"WinlogonManagerService"

WindowsService* WindowsService::s_pThis = nullptr;
std::mutex WindowsService::s_pThisMutex;

WindowsService::WindowsService()
    : m_ServiceStopEvent(INVALID_HANDLE_VALUE)
    , m_IsRunningAsService(false)
    , m_InstanceMutex(INVALID_HANDLE_VALUE)
    , m_WinlogonSuspended(false) {
    std::lock_guard<std::mutex> lock(s_pThisMutex);
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

    // ��������ֱ��ִ��
    if (!m_Command.empty()) {
        return HandleCommand() ? 0 : 1;
    }

    // ���û���������IPC������
    if (!m_IPCManager.StartServer([this](const std::string cmd) {
        return this->HandleIPCCommand(cmd);
        })) {
        std::cout << "����IPC������ʧ��" << std::endl;
        return 1;
    }
    std::cout << "IPC���������������ȴ�����..." << std::endl;

    // �ȴ��û�����򱣳�IPC����������
    std::cout << "��������˳�..." << std::endl;
    std::cin.get();
    return 0;
}

void WINAPI WindowsService::ServiceMain(DWORD argc, LPTSTR* argv) {
    std::lock_guard<std::mutex> lock(s_pThisMutex);
    if (!s_pThis) return;

    // ע�������ƴ�����
    s_pThis->m_ServiceStatusHandle = RegisterServiceCtrlHandler(
        SERVICE_NAME, ServiceCtrlHandler);

    if (!s_pThis->m_ServiceStatusHandle) {
        return;
    }

    // ���������������
    s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    s_pThis->m_ServiceStatus.dwWaitHint = 3000;
    SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);

    // ����ֹͣ�¼�
    s_pThis->m_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (s_pThis->m_ServiceStopEvent == NULL) {
        s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);
        return;
    }

    // ����IPC������
    if (!s_pThis->m_IPCManager.StartServer([](const std::string cmd) {
        if (WindowsService::s_pThis) {
            return WindowsService::s_pThis->HandleIPCCommand(cmd);
        }
        return false;
        })) {
        // IPC����������ʧ�ܣ���������Ȼ��������
        // �����������¼��־
    }

    // ���������������
    s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    s_pThis->m_ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);

    s_pThis->m_IsRunningAsService = true;
    
    // ���з�����ѭ��
    s_pThis->ServiceWorkerThread();

    // ����ֹͣʱ������Դ
    s_pThis->m_IPCManager.StopServer();

    // ���������ֹͣ
    s_pThis->m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(s_pThis->m_ServiceStatusHandle, &s_pThis->m_ServiceStatus);
}

void WINAPI WindowsService::ServiceCtrlHandler(DWORD dwCtrl) {
    std::lock_guard<std::mutex> lock(s_pThisMutex);
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
    while (true) {
        // �ȴ�ֹͣ�¼�����ʱʱ��Ϊ1��
        DWORD waitResult = WaitForSingleObject(m_ServiceStopEvent, 1000);
        
        if (waitResult == WAIT_OBJECT_0) {
            // �յ�ֹͣ�źţ��˳�ѭ��
            break;
        }
        
        // ����������ӷ���Ķ�������
        // ���磺���ϵͳ״̬�����������
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
    else if (m_Command == "--help") {
        std::cout << "Winlogon Manager Service" << std::endl;
        std::cout << "��������:" << std::endl;
        std::cout << "  --install    ��װ����" << std::endl;
        std::cout << "  --uninstall  ж�ط���" << std::endl;
        std::cout << "  --start      ��������" << std::endl;
        std::cout << "  --stop       ֹͣ����" << std::endl;
        std::cout << "  --suspend    ����winlogon����" << std::endl;
        std::cout << "  --resume     �ָ�winlogon����" << std::endl;
        std::cout << "  --status     ��ѯ����״̬" << std::endl;
        std::cout << "  --help       ��ʾ�˰�����Ϣ" << std::endl;
        return true;
    }
    else {
        std::cout << "δ֪����: " << m_Command << std::endl;
        std::cout << "ʹ�� --help �鿴��������" << std::endl;
        return false;
    }
}

bool WindowsService::InstallService() {
    std::cout << "��ʼ��װ����..." << std::endl;
    
    ServiceManager serviceManager(SERVICE_NAME);

    wchar_t modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);
    
    std::cout << "����·��: ";
    std::wcout << modulePath << std::endl;

    bool result = serviceManager.InstallService(
        L"Winlogon Manager Service",
        L"����winlogon���̵ķ���",
        modulePath
    );
    
    if (result) {
        std::cout << "����װ�ɹ���" << std::endl;
    } else {
        std::cout << "����װʧ�ܣ�" << std::endl;
    }
    
    return result;
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
    std::cout << "���� winlogon.exe ʧ�ܣ���ȷ���Թ���ԱȨ������" << std::endl;
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
