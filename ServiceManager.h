#pragma once

#include "Common.h"

class ServiceManager {
public:
    explicit ServiceManager(const WString& serviceName);
    ~ServiceManager();
    
    // 禁用拷贝构造和赋值
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;
    
    // 服务管理功能
    bool InstallService(const WString& displayName, const WString& description, const WString& binaryPath);
    bool UninstallService();
    bool StartService();
    bool StopService();
    bool RestartService();
    bool QueryStatus();
    bool IsInstalled();
    
    // 获取服务信息
    WString GetServiceName() const { return m_serviceName; }
    ServiceState GetServiceState() const;
    String GetServiceStateString() const;
    
    // 错误处理
    ErrorCode GetLastError() const { return m_lastError; }
    String GetLastErrorString() const;
    
private:
    WString m_serviceName;
    SC_HANDLE m_scManager;
    SC_HANDLE m_service;
    ErrorCode m_lastError;
    
    bool OpenService();
    void CloseService();
    void SetLastError(ErrorCode error);
    void SetLastError(DWORD win32Error);
};