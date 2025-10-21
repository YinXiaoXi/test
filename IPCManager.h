#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

class IPCManager {
public:
    IPCManager();
    ~IPCManager();

    // ʹ�ûص���������������
    using CommandHandler = std::function<bool(const std::string&)>;

    // �������˺���
    bool StartServer(CommandHandler handler = nullptr);
    void StopServer();
    bool IsServerRunning() const;

    // �ͻ��˺���
    bool SendCommandToServer(const std::string& command);

    // ͨ�ú���
    static std::string GetPipeName();

private:
    static DWORD WINAPI ServerThread(LPVOID lpParam);
    void ProcessClientCommand(HANDLE hPipe, const std::string& command);

    HANDLE m_hServerThread;
    HANDLE m_hStopEvent;
    bool m_ServerRunning;
    CommandHandler m_CommandHandler;
};
