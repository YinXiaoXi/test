#include "IPCManager.h"
#include <iostream>
#include <sstream>

#define PIPE_NAME L"\\\\.\\pipe\\WinlogonManagerServicePipe"
#define BUFFER_SIZE 4096

IPCManager::IPCManager()
    : m_hServerThread(INVALID_HANDLE_VALUE)
    , m_hStopEvent(INVALID_HANDLE_VALUE)
    , m_ServerRunning(false)
    , m_CommandHandler(nullptr) {

    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

IPCManager::~IPCManager() {
    StopServer();
    if (m_hStopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hStopEvent);
    }
}

std::string IPCManager::GetPipeName() {
    return "\\\\.\\pipe\\WinlogonManagerServicePipe";
}

bool IPCManager::StartServer(CommandHandler handler) {
    if (m_ServerRunning) {
        return true;
    }

    m_CommandHandler = handler;
    ResetEvent(m_hStopEvent);

    m_hServerThread = CreateThread(
        NULL,
        0,
        ServerThread,
        this,
        0,
        NULL
    );

    if (m_hServerThread == NULL) {
        return false;
    }

    m_ServerRunning = true;
    return true;
}

void IPCManager::StopServer() {
    if (!m_ServerRunning) {
        return;
    }

    SetEvent(m_hStopEvent);

    // 发送一个空连接来唤醒服务器线程
    HANDLE hPipe = CreateFile(
        PIPE_NAME,
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

    if (m_hServerThread != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(m_hServerThread, 5000);
        CloseHandle(m_hServerThread);
        m_hServerThread = INVALID_HANDLE_VALUE;
    }

    m_ServerRunning = false;
    m_CommandHandler = nullptr;
}

bool IPCManager::IsServerRunning() const {
    return m_ServerRunning;
}

DWORD WINAPI IPCManager::ServerThread(LPVOID lpParam) {
    IPCManager* pThis = static_cast<IPCManager*>(lpParam);
    if (!pThis) {
        return 1;
    }

    while (WaitForSingleObject(pThis->m_hStopEvent, 0) != WAIT_OBJECT_0) {
        HANDLE hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFFER_SIZE,
            BUFFER_SIZE,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cout << "创建命名管道失败，错误代码: " << GetLastError() << std::endl;
            break;
        }

        // 等待客户端连接（带超时）
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            std::cout << "客户端已连接到IPC管道" << std::endl;
            char buffer[BUFFER_SIZE];
            DWORD bytesRead;

            BOOL success = ReadFile(
                hPipe,
                buffer,
                BUFFER_SIZE - 1,
                &bytesRead,
                NULL
            );

            if (success && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string command(buffer);
                pThis->ProcessClientCommand(hPipe, command);
            }
        }

        CloseHandle(hPipe);
    }

    return 0;
}

void IPCManager::ProcessClientCommand(HANDLE hPipe, const std::string& command) {
    std::cout << "接收到命令: " << command << std::endl;

    std::string response;

    // 使用回调函数处理命令
    if (m_CommandHandler) {
        if (m_CommandHandler(command)) {
            response = "命令执行成功: " + command;
        }
        else {
            response = "命令执行失败: " + command;
        }
    }
    else {
        response = "无命令处理器可用";
    }

    // 发送响应
    DWORD bytesWritten;
    WriteFile(
        hPipe,
        response.c_str(),
        static_cast<DWORD>(response.length()),
        &bytesWritten,
        NULL
    );
}

bool IPCManager::SendCommandToServer(const std::string& command) {
    HANDLE hPipe = CreateFile(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 设置管道模式
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);

    // 发送命令
    DWORD bytesWritten;
    BOOL success = WriteFile(
        hPipe,
        command.c_str(),
        static_cast<DWORD>(command.length()),
        &bytesWritten,
        NULL
    );

    if (!success) {
        CloseHandle(hPipe);
        return false;
    }

    // 读取响应
    char buffer[BUFFER_SIZE];
    DWORD bytesRead;
    success = ReadFile(
        hPipe,
        buffer,
        BUFFER_SIZE - 1,
        &bytesRead,
        NULL
    );

    if (success && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "服务器响应: " << buffer << std::endl;
    }

    CloseHandle(hPipe);
    return true;
}