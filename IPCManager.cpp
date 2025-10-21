#include "IPCManager.h"
#include "Logger.h"
#include "Utils.h"

IPCManager::IPCManager()
    : m_pipeName(L"\\\\.\\pipe\\WinlogonManagerService")
    , m_timeoutMs(5000)
    , m_serverThread(INVALID_HANDLE_VALUE)
    , m_stopEvent(INVALID_HANDLE_VALUE)
    , m_serverRunning(false)
    , m_lastError(ErrorCode::Success) {
    
    m_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!m_stopEvent) {
        Logger::Log(LogLevel::Error, "Failed to create stop event, error: %s", Utils::GetLastErrorString().c_str());
        SetLastError(GetLastError());
    }
}

IPCManager::~IPCManager() {
    StopServer();
    if (m_stopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_stopEvent);
    }
}

bool IPCManager::StartServer(CommandHandler handler) {
    if (m_serverRunning) {
        Logger::Log(LogLevel::Warning, "IPC server is already running");
        return true;
    }
    
    m_commandHandler = handler;
    ResetEvent(m_stopEvent);
    
    m_serverThread = CreateThread(NULL, 0, ServerThreadProc, this, 0, NULL);
    if (!m_serverThread) {
        Logger::Log(LogLevel::Error, "Failed to create server thread, error: %s", Utils::GetLastErrorString().c_str());
        SetLastError(GetLastError());
        return false;
    }
    
    m_serverRunning = true;
    Logger::Log(LogLevel::Info, "IPC server started successfully");
    return true;
}

void IPCManager::StopServer() {
    if (!m_serverRunning) {
        return;
    }
    
    Logger::Log(LogLevel::Info, "Stopping IPC server...");
    SetEvent(m_stopEvent);
    
    // 发送一个连接请求来唤醒服务器线程
    HANDLE hPipe = CreateFileW(
        m_pipeName.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
    }
    
    if (m_serverThread != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(m_serverThread, 5000);
        CloseHandle(m_serverThread);
        m_serverThread = INVALID_HANDLE_VALUE;
    }
    
    m_serverRunning = false;
    m_commandHandler = nullptr;
    Logger::Log(LogLevel::Info, "IPC server stopped");
}

bool IPCManager::SendCommand(const String& command, String& response) {
    HANDLE hPipe = CreateFileW(
        m_pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        Logger::Log(LogLevel::Error, "Failed to connect to IPC server, error: %s", Utils::GetLastErrorString().c_str());
        SetLastError(ErrorCode::IPCConnectionFailed);
        return false;
    }
    
    // 设置管道模式
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(hPipe, &mode, NULL, NULL)) {
        Logger::Log(LogLevel::Error, "Failed to set pipe mode, error: %s", Utils::GetLastErrorString().c_str());
        CloseHandle(hPipe);
        SetLastError(GetLastError());
        return false;
    }
    
    // 发送命令
    DWORD bytesWritten;
    if (!WriteFile(hPipe, command.c_str(), static_cast<DWORD>(command.length()), &bytesWritten, NULL)) {
        Logger::Log(LogLevel::Error, "Failed to send command, error: %s", Utils::GetLastErrorString().c_str());
        CloseHandle(hPipe);
        SetLastError(GetLastError());
        return false;
    }
    
    // 读取响应
    char buffer[4096];
    DWORD bytesRead;
    if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        Logger::Log(LogLevel::Error, "Failed to read response, error: %s", Utils::GetLastErrorString().c_str());
        CloseHandle(hPipe);
        SetLastError(GetLastError());
        return false;
    }
    
    buffer[bytesRead] = '\0';
    response = String(buffer);
    
    CloseHandle(hPipe);
    Logger::Log(LogLevel::Debug, "Command sent successfully, response: %s", response.c_str());
    return true;
}

bool IPCManager::SendCommand(const String& command) {
    String response;
    return SendCommand(command, response);
}

void IPCManager::SetPipeName(const WString& pipeName) {
    if (m_serverRunning) {
        Logger::Log(LogLevel::Warning, "Cannot change pipe name while server is running");
        return;
    }
    m_pipeName = pipeName;
}

String IPCManager::GetLastErrorString() const {
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

DWORD WINAPI IPCManager::ServerThreadProc(LPVOID lpParam) {
    IPCManager* pThis = static_cast<IPCManager*>(lpParam);
    if (!pThis) {
        return 1;
    }
    
    Logger::Log(LogLevel::Info, "IPC server thread started");
    
    while (WaitForSingleObject(pThis->m_stopEvent, 100) != WAIT_OBJECT_0) {
        HANDLE hPipe = CreateNamedPipeW(
            pThis->m_pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            NULL
        );
        
        if (hPipe == INVALID_HANDLE_VALUE) {
            Logger::Log(LogLevel::Error, "Failed to create named pipe, error: %s", Utils::GetLastErrorString().c_str());
            break;
        }
        
        // 等待客户端连接
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? 
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        
        if (connected) {
            Logger::Log(LogLevel::Debug, "Client connected to IPC pipe");
            
            char buffer[4096];
            DWORD bytesRead;
            
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                String command(buffer);
                pThis->ProcessClientCommand(hPipe, command);
            }
        }
        
        CloseHandle(hPipe);
    }
    
    Logger::Log(LogLevel::Info, "IPC server thread stopped");
    return 0;
}

void IPCManager::ProcessClientCommand(HANDLE hPipe, const String& command) {
    Logger::Log(LogLevel::Info, "Processing command: %s", command.c_str());
    
    String response;
    
    if (m_commandHandler) {
        if (m_commandHandler(command)) {
            response = "Command executed successfully: " + command;
        } else {
            response = "Command execution failed: " + command;
        }
    } else {
        response = "No command handler available";
    }
    
    // 发送响应
    DWORD bytesWritten;
    WriteFile(hPipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, NULL);
    
    Logger::Log(LogLevel::Debug, "Response sent: %s", response.c_str());
}

void IPCManager::SetLastError(ErrorCode error) {
    m_lastError = error;
}

void IPCManager::SetLastError(DWORD win32Error) {
    switch (win32Error) {
        case ERROR_ACCESS_DENIED: m_lastError = ErrorCode::AccessDenied; break;
        case ERROR_INVALID_PARAMETER: m_lastError = ErrorCode::InvalidParameter; break;
        case ERROR_PIPE_NOT_CONNECTED: m_lastError = ErrorCode::IPCConnectionFailed; break;
        default: m_lastError = ErrorCode::UnknownError; break;
    }
}