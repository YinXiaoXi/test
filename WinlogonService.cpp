#include "WinlogonService.h"

// 静态成员定义
WinlogonService* WinlogonService::s_instance = nullptr;
std::mutex WinlogonService::s_instanceMutex;

WinlogonService::WinlogonService()
    : m_serviceStatusHandle(NULL)
    , m_serviceStopEvent(INVALID_HANDLE_VALUE)
    , m_isRunningAsService(false)
    , m_winlogonSuspended(false)
    , m_lastError(ErrorCode::Success) {
    
    // 初始化服务状态
    m_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_serviceStatus.dwCurrentState = SERVICE_STOPPED;
    m_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_serviceStatus.dwWin32ExitCode = NO_ERROR;
    m_serviceStatus.dwServiceSpecificExitCode = 0;
    m_serviceStatus.dwCheckPoint = 0;
    m_serviceStatus.dwWaitHint = 0;
    
    // 创建管理器
    m_serviceManager = std::make_unique<ServiceManager>(L"WinlogonManagerService");
    m_processManager = std::make_unique<ProcessManager>();
    m_ipcManager = std::make_unique<IPCManager>();
    
    // 设置实例
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    s_instance = this;
}

WinlogonService::~WinlogonService() {
    CleanupService();
    
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    s_instance = nullptr;
}

int WinlogonService::Run(int argc, char* argv[]) {
    Logger::Initialize();
    Logger::SetLogLevel(LogLevel::Info);
    
    if (!ParseCommandLine(argc, argv)) {
        Logger::Log(LogLevel::Info, "No parameters provided, using default behavior...");
    }
    
    // 检查是否作为服务运行
    if (argc > 1 && strcmp(argv[1], "--service") == 0) {
        Logger::Log(LogLevel::Info, "Starting as Windows service...");
        
        SERVICE_TABLE_ENTRYW serviceTable[] = {
            { (LPWSTR)L"WinlogonManagerService", ServiceMain },
            { NULL, NULL }
        };
        
        if (!StartServiceCtrlDispatcherW(serviceTable)) {
            Logger::Log(LogLevel::Error, "Failed to start service control dispatcher, error: %s", Utils::GetLastErrorString().c_str());
            return 1;
        }
        return 0;
    }
    else {
        Logger::Log(LogLevel::Info, "Starting as console application...");
        
        // 如果有命令，直接执行
        if (!m_currentCommand.empty()) {
            return HandleCommand(m_currentCommand) ? 0 : 1;
        }
        
        // 启动IPC服务器
        if (!m_ipcManager->StartServer([this](const String& cmd) {
            return this->HandleIPCCommand(cmd);
        })) {
            Logger::Log(LogLevel::Error, "Failed to start IPC server");
            return 1;
        }
        
        Logger::Log(LogLevel::Info, "IPC server started, waiting for commands...");
        Logger::Log(LogLevel::Info, "Press any key to exit...");
        std::cin.get();
        
        m_ipcManager->StopServer();
        return 0;
    }
}

bool WinlogonService::ParseCommandLine(int argc, char* argv[]) {
    if (argc > 1) {
        m_currentCommand = argv[1];
        return true;
    }
    return false;
}

bool WinlogonService::HandleCommand(const String& command) {
    Logger::Log(LogLevel::Info, "Handling command: %s", command.c_str());
    
    if (command == "--install") {
        return InstallService();
    }
    else if (command == "--uninstall") {
        return UninstallService();
    }
    else if (command == "--start") {
        return StartService();
    }
    else if (command == "--stop") {
        return StopService();
    }
    else if (command == "--restart") {
        return RestartService();
    }
    else if (command == "--status") {
        return QueryServiceStatus();
    }
    else if (command == "--suspend") {
        return SuspendWinlogon();
    }
    else if (command == "--resume") {
        return ResumeWinlogon();
    }
    else if (command == "--winlogon-status") {
        return QueryWinlogonStatus();
    }
    else if (command == "--help") {
        ShowHelp();
        return true;
    }
    else {
        Logger::Log(LogLevel::Warning, "Unknown command: %s", command.c_str());
        ShowHelp();
        return false;
    }
}

void WinlogonService::ShowHelp() {
    std::cout << "Winlogon Manager Service v2.0" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  --install         Install service" << std::endl;
    std::cout << "  --uninstall       Uninstall service" << std::endl;
    std::cout << "  --start           Start service" << std::endl;
    std::cout << "  --stop            Stop service" << std::endl;
    std::cout << "  --restart         Restart service" << std::endl;
    std::cout << "  --status          Query service status" << std::endl;
    std::cout << "  --suspend         Suspend winlogon process" << std::endl;
    std::cout << "  --resume          Resume winlogon process" << std::endl;
    std::cout << "  --winlogon-status Query winlogon process status" << std::endl;
    std::cout << "  --help            Show this help message" << std::endl;
}

bool WinlogonService::InstallService() {
    Logger::Log(LogLevel::Info, "Installing service...");
    
    WString modulePath = Utils::StringToWString(Utils::GetModulePath());
    WString servicePath = modulePath + L" --service";
    
    bool result = m_serviceManager->InstallService(
        L"Winlogon Manager Service",
        L"Manages winlogon process operations",
        servicePath
    );
    
    if (result) {
        Logger::Log(LogLevel::Info, "Service installed successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to install service: %s", m_serviceManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::UninstallService() {
    Logger::Log(LogLevel::Info, "Uninstalling service...");
    
    bool result = m_serviceManager->UninstallService();
    
    if (result) {
        Logger::Log(LogLevel::Info, "Service uninstalled successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to uninstall service: %s", m_serviceManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::StartService() {
    Logger::Log(LogLevel::Info, "Starting service...");
    
    bool result = m_serviceManager->StartService();
    
    if (result) {
        Logger::Log(LogLevel::Info, "Service started successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to start service: %s", m_serviceManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::StopService() {
    Logger::Log(LogLevel::Info, "Stopping service...");
    
    bool result = m_serviceManager->StopService();
    
    if (result) {
        Logger::Log(LogLevel::Info, "Service stopped successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to stop service: %s", m_serviceManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::RestartService() {
    Logger::Log(LogLevel::Info, "Restarting service...");
    
    bool result = m_serviceManager->RestartService();
    
    if (result) {
        Logger::Log(LogLevel::Info, "Service restarted successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to restart service: %s", m_serviceManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::QueryServiceStatus() {
    Logger::Log(LogLevel::Info, "Querying service status...");
    
    bool result = m_serviceManager->QueryStatus();
    
    if (result) {
        Logger::Log(LogLevel::Info, "Service status: %s", m_serviceManager->GetServiceStateString().c_str());
    } else {
        Logger::Log(LogLevel::Error, "Failed to query service status: %s", m_serviceManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::SuspendWinlogon() {
    Logger::Log(LogLevel::Info, "Suspending winlogon process...");
    
    if (!Utils::IsRunningAsAdmin()) {
        Logger::Log(LogLevel::Error, "Administrator privileges required to suspend winlogon process");
        SetLastError(ErrorCode::AccessDenied);
        return false;
    }
    
    bool result = m_processManager->SuspendProcess(L"winlogon.exe");
    
    if (result) {
        m_winlogonSuspended = true;
        Logger::Log(LogLevel::Info, "Winlogon process suspended successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to suspend winlogon process: %s", m_processManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::ResumeWinlogon() {
    Logger::Log(LogLevel::Info, "Resuming winlogon process...");
    
    if (!Utils::IsRunningAsAdmin()) {
        Logger::Log(LogLevel::Error, "Administrator privileges required to resume winlogon process");
        SetLastError(ErrorCode::AccessDenied);
        return false;
    }
    
    bool result = m_processManager->ResumeProcess(L"winlogon.exe");
    
    if (result) {
        m_winlogonSuspended = false;
        Logger::Log(LogLevel::Info, "Winlogon process resumed successfully");
    } else {
        Logger::Log(LogLevel::Error, "Failed to resume winlogon process: %s", m_processManager->GetLastErrorString().c_str());
    }
    
    return result;
}

bool WinlogonService::QueryWinlogonStatus() {
    Logger::Log(LogLevel::Info, "Querying winlogon process status...");
    
    bool isRunning = m_processManager->IsProcessRunning(L"winlogon.exe");
    
    if (isRunning) {
        Logger::Log(LogLevel::Info, "Winlogon process is running");
        if (m_winlogonSuspended) {
            Logger::Log(LogLevel::Info, "Winlogon process is suspended");
        } else {
            Logger::Log(LogLevel::Info, "Winlogon process is active");
        }
    } else {
        Logger::Log(LogLevel::Warning, "Winlogon process is not running");
    }
    
    return true;
}

String WinlogonService::GetLastErrorString() const {
    switch (m_lastError) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::InvalidParameter: return "Invalid parameter";
        case ErrorCode::AccessDenied: return "Access denied";
        case ErrorCode::ServiceNotFound: return "Service not found";
        case ErrorCode::ProcessNotFound: return "Process not found";
        case ErrorCode::IPCConnectionFailed: return "IPC connection failed";
        case ErrorCode::UnknownError: return "Unknown error";
        default: return "Unknown error";
    }
}

void WINAPI WinlogonService::ServiceMain(DWORD argc, LPTSTR* argv) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        Logger::Log(LogLevel::Error, "Service instance not available");
        return;
    }
    
    Logger::Log(LogLevel::Info, "Service main function called");
    
    // 注册服务控制处理器
    s_instance->m_serviceStatusHandle = RegisterServiceCtrlHandlerW(
        L"WinlogonManagerService", ServiceCtrlHandler);
    
    if (!s_instance->m_serviceStatusHandle) {
        Logger::Log(LogLevel::Error, "Failed to register service control handler, error: %s", Utils::GetLastErrorString().c_str());
        return;
    }
    
    // 设置服务启动状态
    s_instance->UpdateServiceStatus(SERVICE_START_PENDING, 3000);
    
    // 创建停止事件
    s_instance->m_serviceStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!s_instance->m_serviceStopEvent) {
        Logger::Log(LogLevel::Error, "Failed to create stop event, error: %s", Utils::GetLastErrorString().c_str());
        s_instance->UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }
    
    // 启动IPC服务器
    if (!s_instance->m_ipcManager->StartServer([s_instance](const String& cmd) {
        return s_instance->HandleIPCCommand(cmd);
    })) {
        Logger::Log(LogLevel::Warning, "Failed to start IPC server, but service will continue running");
    }
    
    // 设置服务运行状态
    s_instance->UpdateServiceStatus(SERVICE_RUNNING);
    s_instance->m_isRunningAsService = true;
    
    Logger::Log(LogLevel::Info, "Service started successfully");
    
    // 运行服务循环
    s_instance->ServiceWorkerThread();
    
    // 清理资源
    s_instance->CleanupService();
    
    // 设置服务停止
    s_instance->UpdateServiceStatus(SERVICE_STOPPED);
    
    Logger::Log(LogLevel::Info, "Service stopped");
}

void WINAPI WinlogonService::ServiceCtrlHandler(DWORD dwCtrl) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        return;
    }
    
    switch (dwCtrl) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            Logger::Log(LogLevel::Info, "Service stop requested");
            s_instance->UpdateServiceStatus(SERVICE_STOP_PENDING);
            SetEvent(s_instance->m_serviceStopEvent);
            break;
        default:
            break;
    }
}

bool WinlogonService::HandleIPCCommand(const String& command) {
    return HandleCommand(command);
}

void WinlogonService::InitializeService() {
    // 服务初始化逻辑
}

void WinlogonService::CleanupService() {
    Logger::Log(LogLevel::Info, "Cleaning up service...");
    
    if (m_ipcManager) {
        m_ipcManager->StopServer();
    }
    
    if (m_serviceStopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_serviceStopEvent);
        m_serviceStopEvent = INVALID_HANDLE_VALUE;
    }
}

void WinlogonService::ServiceWorkerThread() {
    Logger::Log(LogLevel::Info, "Service worker thread started");
    
    while (true) {
        DWORD waitResult = WaitForSingleObject(m_serviceStopEvent, 1000);
        
        if (waitResult == WAIT_OBJECT_0) {
            Logger::Log(LogLevel::Info, "Service stop signal received");
            break;
        }
        
        // 这里可以添加定期检查的任务
        // 例如：检查系统状态、处理请求等
    }
    
    Logger::Log(LogLevel::Info, "Service worker thread stopped");
}

void WinlogonService::SetLastError(ErrorCode error) {
    m_lastError = error;
}

void WinlogonService::SetLastError(DWORD win32Error) {
    switch (win32Error) {
        case ERROR_ACCESS_DENIED: m_lastError = ErrorCode::AccessDenied; break;
        case ERROR_INVALID_PARAMETER: m_lastError = ErrorCode::InvalidParameter; break;
        case ERROR_SERVICE_DOES_NOT_EXIST: m_lastError = ErrorCode::ServiceNotFound; break;
        case ERROR_PROCESS_NOT_FOUND: m_lastError = ErrorCode::ProcessNotFound; break;
        default: m_lastError = ErrorCode::UnknownError; break;
    }
}

void WinlogonService::UpdateServiceStatus(DWORD currentState, DWORD waitHint) {
    m_serviceStatus.dwCurrentState = currentState;
    m_serviceStatus.dwWaitHint = waitHint;
    
    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED) {
        m_serviceStatus.dwCheckPoint = 0;
    } else {
        m_serviceStatus.dwCheckPoint++;
    }
    
    SetServiceStatus(m_serviceStatusHandle, &m_serviceStatus);
}