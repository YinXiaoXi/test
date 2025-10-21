#include "ServiceManager.h"
#include <iostream>

ServiceManager::ServiceManager(const wchar_t* serviceName) 
: m_ServiceName(serviceName)
, m_SCManager(NULL)
, m_Service(NULL) {

	m_SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!m_SCManager) {
		DWORD error = GetLastError();
		std::cout << "无法打开服务控制管理器，错误代码: " << error << std::endl;
		if (error == ERROR_ACCESS_DENIED) {
			std::cout << "权限不足！请以管理员身份运行此程序。" << std::endl;
		}
	}
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
		std::wcout << L"无法打开服务控制管理器，请以管理员权限运行" << std::endl;
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
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS) {
			std::cout << "服务已存在: " << std::endl;
			return true;
		}
		if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
			std::cout << "服务已标记为删除，正在等待删除完成..." << std::endl;
			// 等待服务删除完成
			for (int i = 0; i < 15; i++) {
				Sleep(2000);
				std::cout << "等待中... " << (i+1) << "/15" << std::endl;
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
				if (m_Service) {
					std::cout << "服务重新创建成功" << std::endl;
					break;
				}
				DWORD newError = GetLastError();
				if (newError != ERROR_SERVICE_MARKED_FOR_DELETE && newError != ERROR_SERVICE_EXISTS) {
					std::cout << "等待过程中出现新错误: " << newError << std::endl;
					break;
				}
			}
			if (!m_Service) {
				std::cout << "等待服务删除超时，请手动删除服务或重启系统" << std::endl;
				return false;
			}
		}
		std::cout << "创建服务失败，错误代码: " << error << std::endl;
		
		// 显示具体的错误信息
		switch (error) {
		case ERROR_ACCESS_DENIED:
			std::cout << "错误: 访问被拒绝，请以管理员身份运行" << std::endl;
			break;
		case ERROR_INVALID_NAME:
			std::cout << "错误: 服务名称无效" << std::endl;
			break;
		case ERROR_INVALID_PARAMETER:
			std::cout << "错误: 参数无效" << std::endl;
			break;
		case ERROR_PATH_NOT_FOUND:
			std::cout << "错误: 服务路径不存在" << std::endl;
			break;
		case ERROR_SERVICE_MARKED_FOR_DELETE:
			std::cout << "错误: 服务已标记为删除，请等待或重启系统" << std::endl;
			break;
		case ERROR_SERVICE_EXISTS:
			std::cout << "错误: 服务已存在" << std::endl;
			break;
		default:
			std::cout << "错误: 未知错误 (代码: " << error << ")" << std::endl;
			break;
		}
		return false;
	}

	// 设置服务描述
	SERVICE_DESCRIPTION sd = { (LPWSTR)description };
	ChangeServiceConfig2(m_Service, SERVICE_CONFIG_DESCRIPTION, &sd);

	// 验证服务配置
	SERVICE_STATUS status;
	if (QueryServiceStatus(m_Service, &status)) {
		std::cout << "服务安装成功: " << std::endl;
		std::cout << "服务类型: WIN32_OWN_PROCESS" << std::endl;
		std::cout << "启动类型: 自动启动" << std::endl;
	} else {
		std::cout << "服务安装成功，但配置验证失败" << std::endl;
	}

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