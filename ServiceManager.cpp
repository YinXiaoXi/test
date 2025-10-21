#include "ServiceManager.h"

ServiceManager::ServiceManager(const WString& serviceName)
    : m_serviceName(serviceName)
    , m_scManager(NULL)
    , m_service(NULL)
    , m_lastError(ErrorCode::Success) {
    
    m_scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!m_scManager) {
        DWORD error = GetLastError();
        Logger::Log(LogLevel::Error, "Failed to open service control manager, error: %d", error);
        SetLastError(error);
        
        if (error == ERROR_ACCESS_DENIED) {
            Logger::Log(LogLevel::Error, "Access denied. Please run as administrator.");
        }
    }
}

ServiceManager::~ServiceManager() {
    CloseService();
    if (m_scManager) {
        CloseServiceHandle(m_scManager);
    }
}

bool ServiceManager::InstallService(const WString& displayName, const WString& description, const WString& binaryPath) {
    if (!m_scManager) {
        Logger::Log(LogLevel::Error, "Service control manager not available");
        SetLastError(ErrorCode::AccessDenied);
        return false;
    }
    
    // 检查服务是否已存在
    if (IsInstalled()) {
        Logger::Log(LogLevel::Info, "Service already exists: %s", Utils::WStringToString(m_serviceName).c_str());
        return true;
    }
    
    m_service = CreateServiceW(
        m_scManager,
        m_serviceName.c_str(),
        displayName.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        binaryPath.c_str(),
        NULL, NULL, NULL, NULL, NULL
    );
    
    if (!m_service) {
        DWORD error = GetLastError();
        Logger::Log(LogLevel::Error, "Failed to create service, error: %d", error);
        SetLastError(error);
        
        switch (error) {
            case ERROR_SERVICE_EXISTS:
                Logger::Log(LogLevel::Info, "Service already exists");
                return true;
            case ERROR_ACCESS_DENIED:
                Logger::Log(LogLevel::Error, "Access denied. Please run as administrator.");
                break;
            case ERROR_INVALID_NAME:
                Logger::Log(LogLevel::Error, "Invalid service name");
                break;
            case ERROR_INVALID_PARAMETER:
                Logger::Log(LogLevel::Error, "Invalid parameter");
                break;
            case ERROR_PATH_NOT_FOUND:
                Logger::Log(LogLevel::Error, "Binary path not found");
                break;
            default:
                Logger::Log(LogLevel::Error, "Unknown error: %d", error);
                break;
        }
        return false;
    }
    
    // 设置服务描述
    SERVICE_DESCRIPTIONW sd = { const_cast<LPWSTR>(description.c_str()) };
    ChangeServiceConfig2W(m_service, SERVICE_CONFIG_DESCRIPTION, &sd);
    
    Logger::Log(LogLevel::Info, "Service installed successfully: %s", Utils::WStringToString(m_serviceName).c_str());
    CloseService();
    return true;
}

bool ServiceManager::UninstallService() {
    if (!OpenService()) {
        Logger::Log(LogLevel::Info, "Service not found: %s", Utils::WStringToString(m_serviceName).c_str());
        return true; // 服务不存在，认为卸载成功
    }
    
    // 停止服务
    SERVICE_STATUS status;
    if (ControlService(m_service, SERVICE_CONTROL_STOP, &status)) {
        Logger::Log(LogLevel::Info, "Stopping service...");
        Sleep(1000); // 等待服务停止
    }
    
    if (!DeleteService(m_service)) {
        DWORD error = GetLastError();
        Logger::Log(LogLevel::Error, "Failed to delete service, error: %d", error);
        SetLastError(error);
        CloseService();
        return false;
    }
    
    Logger::Log(LogLevel::Info, "Service uninstalled successfully: %s", Utils::WStringToString(m_serviceName).c_str());
    CloseService();
    return true;
}

bool ServiceManager::StartService() {
    if (!OpenService()) {
        SetLastError(ErrorCode::ServiceNotFound);
        return false;
    }
    
    if (!::StartServiceW(m_service, 0, NULL)) {
        DWORD error = GetLastError();
        Logger::Log(LogLevel::Error, "Failed to start service, error: %d", error);
        SetLastError(error);
        CloseService();
        
        if (error == ERROR_SERVICE_ALREADY_RUNNING) {
            Logger::Log(LogLevel::Info, "Service is already running: %s", Utils::WStringToString(m_serviceName).c_str());
            return true;
        }
        return false;
    }
    
    Logger::Log(LogLevel::Info, "Service started successfully: %s", Utils::WStringToString(m_serviceName).c_str());
    CloseService();
    return true;
}

bool ServiceManager::StopService() {
    if (!OpenService()) {
        SetLastError(ErrorCode::ServiceNotFound);
        return false;
    }
    
    SERVICE_STATUS status;
    if (!ControlService(m_service, SERVICE_CONTROL_STOP, &status)) {
        DWORD error = GetLastError();
        Logger::Log(LogLevel::Error, "Failed to stop service, error: %d", error);
        SetLastError(error);
        CloseService();
        return false;
    }
    
    Logger::Log(LogLevel::Info, "Service stopped successfully: %s", Utils::WStringToString(m_serviceName).c_str());
    CloseService();
    return true;
}

bool ServiceManager::RestartService() {
    if (!StopService()) {
        return false;
    }
    
    Sleep(2000); // 等待服务完全停止
    
    return StartService();
}

bool ServiceManager::QueryStatus() {
    if (!OpenService()) {
        SetLastError(ErrorCode::ServiceNotFound);
        return false;
    }
    
    SERVICE_STATUS status;
    if (!QueryServiceStatus(m_service, &status)) {
        DWORD error = GetLastError();
        Logger::Log(LogLevel::Error, "Failed to query service status, error: %d", error);
        SetLastError(error);
        CloseService();
        return false;
    }
    
    Logger::Log(LogLevel::Info, "Service status: %s", GetServiceStateString().c_str());
    CloseService();
    return true;
}

bool ServiceManager::IsInstalled() {
    if (!OpenService()) {
        return false;
    }
    CloseService();
    return true;
}

ServiceState ServiceManager::GetServiceState() const {
    if (!m_service) {
        return ServiceState::Stopped;
    }
    
    SERVICE_STATUS status;
    if (!QueryServiceStatus(m_service, &status)) {
        return ServiceState::Stopped;
    }
    
    switch (status.dwCurrentState) {
        case SERVICE_STOPPED: return ServiceState::Stopped;
        case SERVICE_START_PENDING: return ServiceState::Starting;
        case SERVICE_RUNNING: return ServiceState::Running;
        case SERVICE_STOP_PENDING: return ServiceState::Stopping;
        default: return ServiceState::Stopped;
    }
}

String ServiceManager::GetServiceStateString() const {
    switch (GetServiceState()) {
        case ServiceState::Stopped: return "Stopped";
        case ServiceState::Starting: return "Starting";
        case ServiceState::Running: return "Running";
        case ServiceState::Stopping: return "Stopping";
        default: return "Unknown";
    }
}

String ServiceManager::GetLastErrorString() const {
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

bool ServiceManager::OpenService() {
    if (m_service) {
        CloseServiceHandle(m_service);
    }
    
    m_service = ::OpenServiceW(m_scManager, m_serviceName.c_str(), SERVICE_ALL_ACCESS);
    return m_service != NULL;
}

void ServiceManager::CloseService() {
    if (m_service) {
        CloseServiceHandle(m_service);
        m_service = NULL;
    }
}

void ServiceManager::SetLastError(ErrorCode error) {
    m_lastError = error;
}

void ServiceManager::SetLastError(DWORD win32Error) {
    switch (win32Error) {
        case ERROR_ACCESS_DENIED: m_lastError = ErrorCode::AccessDenied; break;
        case ERROR_SERVICE_DOES_NOT_EXIST: m_lastError = ErrorCode::ServiceNotFound; break;
        case ERROR_INVALID_PARAMETER: m_lastError = ErrorCode::InvalidParameter; break;
        default: m_lastError = ErrorCode::UnknownError; break;
    }
}