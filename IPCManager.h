#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

class IPCManager {
public:
    IPCManager();
    ~IPCManager();

    // 使用回调函数来处理命令
    using CommandHandler = std::function<bool(const std::string&)>;

    // 服务器端函数
    bool StartServer(CommandHandler handler = nullptr);
    void StopServer();
    bool IsServerRunning() const;

    // 客户端函数
    bool SendCommandToServer(const std::string& command);

    // 通用函数
    static std::string GetPipeName();

private:
    static DWORD WINAPI ServerThread(LPVOID lpParam);
    void ProcessClientCommand(HANDLE hPipe, const std::string& command);

    HANDLE m_hServerThread;
    HANDLE m_hStopEvent;
    bool m_ServerRunning;
    CommandHandler m_CommandHandler;
};
