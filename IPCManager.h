#pragma once

#include "Common.h"

class IPCManager {
public:
    IPCManager();
    ~IPCManager();
    
    // 禁用拷贝构造和赋值
    IPCManager(const IPCManager&) = delete;
    IPCManager& operator=(const IPCManager&) = delete;
    
    // 服务器功能
    bool StartServer(CommandHandler handler = nullptr);
    void StopServer();
    bool IsServerRunning() const { return m_serverRunning; }
    
    // 客户端功能
    bool SendCommand(const String& command, String& response);
    bool SendCommand(const String& command);
    
    // 配置
    void SetPipeName(const WString& pipeName);
    WString GetPipeName() const { return m_pipeName; }
    void SetTimeout(DWORD timeoutMs) { m_timeoutMs = timeoutMs; }
    DWORD GetTimeout() const { return m_timeoutMs; }
    
    // 错误处理
    ErrorCode GetLastError() const { return m_lastError; }
    String GetLastErrorString() const;
    
private:
    WString m_pipeName;
    DWORD m_timeoutMs;
    HANDLE m_serverThread;
    HANDLE m_stopEvent;
    std::atomic<bool> m_serverRunning;
    CommandHandler m_commandHandler;
    ErrorCode m_lastError;
    
    static DWORD WINAPI ServerThreadProc(LPVOID lpParam);
    void ProcessClientCommand(HANDLE hPipe, const String& command);
    void SetLastError(ErrorCode error);
    void SetLastError(DWORD win32Error);
};