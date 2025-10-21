#include "ServiceManager.h"
#include <iostream>

ServiceManager::ServiceManager(const wchar_t* serviceName) 
: m_ServiceName(serviceName)
, m_SCManager(NULL)
, m_Service(NULL) {

	m_SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
}

ServiceManager::~ServiceManager() {
	CloseService();
	if (m_SCManager) {
		CloseServiceHandle(m_SCManager);
	}
}

bool ServiceManager::OpenService() {
	if (m_Service) {
		CloseServiceHandle(m_Service);
	}

	m_Service = ::OpenService(m_SCManager, m_ServiceName.c_str(), SERVICE_ALL_ACCESS);
	return m_Service != NULL;
}

void ServiceManager::CloseService() {
	if (m_Service) {
		CloseServiceHandle(m_Service);
		m_Service = NULL;
	}
}

bool ServiceManager::InstallService(const wchar_t* displayName, const wchar_t* description, const wchar_t* binaryPath) {
	if (!m_SCManager) {
		return false;
	}

	m_Service = CreateService(
		m_SCManager,
		m_ServiceName.c_str(),
		displayName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		binaryPath,
		NULL, NULL, NULL, NULL, NULL
		);

	if (!m_Service) {
		if (GetLastError() == ERROR_SERVICE_EXISTS) {
			std::wcout << L"服务已存在: " << m_ServiceName << std::endl;
			return true;
		}
		return false;
	}

	// 设置服务描述
	SERVICE_DESCRIPTION sd = { (LPWSTR)description };
	ChangeServiceConfig2(m_Service, SERVICE_CONFIG_DESCRIPTION, &sd);

	std::wcout << L"服务安装成功: " << m_ServiceName << std::endl;
	CloseService();
	return true;
}

bool ServiceManager::UninstallService() {
	if (!OpenService()) {
		std::wcout << L"服务不存在: " << m_ServiceName << std::endl;
		return true;
	}

	SERVICE_STATUS status;
	if (ControlService(m_Service, SERVICE_CONTROL_STOP, &status)) {
		std::wcout << L"正在停止服务..." << std::endl;
		Sleep(1000);
	}

	if (!DeleteService(m_Service)) {
		CloseService();
		return false;
	}

	std::wcout << L"服务卸载成功: " << m_ServiceName << std::endl;
	CloseService();
	return true;
}

bool ServiceManager::StartService() {
	if (!OpenService()) {
		return false;
	}

	if (!::StartService(m_Service, 0, NULL)) {
		DWORD error = GetLastError();
		CloseService();
		if (error == ERROR_SERVICE_ALREADY_RUNNING) {
			std::wcout << L"服务已在运行: " << m_ServiceName << std::endl;
			return true;
		}
		return false;
	}

	std::wcout << L"服务启动成功: " << m_ServiceName << std::endl;
	CloseService();
	return true;
}

bool ServiceManager::StopService() {
	if (!OpenService()) {
		return false;
	}

	SERVICE_STATUS status;
	if (!ControlService(m_Service, SERVICE_CONTROL_STOP, &status)) {
		CloseService();
		return false;
	}

	std::wcout << L"服务停止成功: " << m_ServiceName << std::endl;
	CloseService();
	return true;
}

bool ServiceManager::QueryStatus() {
	if (!OpenService()) {
		std::wcout << L"服务未安装: " << m_ServiceName << std::endl;
		return false;
	}

	SERVICE_STATUS status;
	if (!QueryServiceStatus(m_Service, &status)) {
		CloseService();
		return false;
	}

	std::wcout << L"服务状态: ";
	switch (status.dwCurrentState) {
	case SERVICE_STOPPED:
		std::wcout << L"已停止";
		break;
	case SERVICE_START_PENDING:
		std::wcout << L"启动中";
		break;
	case SERVICE_STOP_PENDING:
		std::wcout << L"停止中";
		break;
	case SERVICE_RUNNING:
		std::wcout << L"运行中";
		break;
	default:
		std::wcout << L"未知状态";
		break;
	}
	std::wcout << std::endl;

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