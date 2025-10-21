#pragma once

#include <windows.h>
#include <string>

class ServiceManager {
public:
	ServiceManager(const wchar_t* serviceName);
	~ServiceManager();

	bool InstallService(const wchar_t* displayName, const wchar_t* description, const wchar_t* binaryPath);
	bool UninstallService();
	bool StartService();
	bool StopService();
	bool QueryStatus();
	bool IsInstalled();

private:
	std::wstring m_ServiceName;
	SC_HANDLE m_SCManager;
	SC_HANDLE m_Service;

	bool OpenService();
	void CloseService();
};