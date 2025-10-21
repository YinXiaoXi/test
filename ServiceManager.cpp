#include "ServiceManager.h"
#include <iostream>

ServiceManager::ServiceManager(const wchar_t* serviceName) 
: m_ServiceName(serviceName)
, m_SCManager(NULL)
, m_Service(NULL) {

	m_SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!m_SCManager) {
		DWORD error = GetLastError();
		std::cout << "�޷��򿪷�����ƹ��������������: " << error << std::endl;
		if (error == ERROR_ACCESS_DENIED) {
			std::cout << "Ȩ�޲��㣡���Թ���Ա������д˳���" << std::endl;
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
		std::wcout << L"�޷��򿪷�����ƹ����������Թ���ԱȨ������" << std::endl;
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
			std::cout << "�����Ѵ���: " << std::endl;
			return true;
		}
		if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
			std::cout << "�����ѱ��Ϊɾ�������ڵȴ�ɾ�����..." << std::endl;
			// �ȴ�����ɾ�����
			for (int i = 0; i < 15; i++) {
				Sleep(2000);
				std::cout << "�ȴ���... " << (i+1) << "/15" << std::endl;
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
					std::cout << "�������´����ɹ�" << std::endl;
					break;
				}
				DWORD newError = GetLastError();
				if (newError != ERROR_SERVICE_MARKED_FOR_DELETE && newError != ERROR_SERVICE_EXISTS) {
					std::cout << "�ȴ������г����´���: " << newError << std::endl;
					break;
				}
			}
			if (!m_Service) {
				std::cout << "�ȴ�����ɾ����ʱ�����ֶ�ɾ�����������ϵͳ" << std::endl;
				return false;
			}
		}
		std::cout << "��������ʧ�ܣ��������: " << error << std::endl;
		
		// ��ʾ����Ĵ�����Ϣ
		switch (error) {
		case ERROR_ACCESS_DENIED:
			std::cout << "����: ���ʱ��ܾ������Թ���Ա�������" << std::endl;
			break;
		case ERROR_INVALID_NAME:
			std::cout << "����: ����������Ч" << std::endl;
			break;
		case ERROR_INVALID_PARAMETER:
			std::cout << "����: ������Ч" << std::endl;
			break;
		case ERROR_PATH_NOT_FOUND:
			std::cout << "����: ����·��������" << std::endl;
			break;
		case ERROR_SERVICE_MARKED_FOR_DELETE:
			std::cout << "����: �����ѱ��Ϊɾ������ȴ�������ϵͳ" << std::endl;
			break;
		case ERROR_SERVICE_EXISTS:
			std::cout << "����: �����Ѵ���" << std::endl;
			break;
		default:
			std::cout << "����: δ֪���� (����: " << error << ")" << std::endl;
			break;
		}
		return false;
	}

	// ���÷�������
	SERVICE_DESCRIPTION sd = { (LPWSTR)description };
	ChangeServiceConfig2(m_Service, SERVICE_CONFIG_DESCRIPTION, &sd);

	// ��֤��������
	SERVICE_STATUS status;
	if (QueryServiceStatus(m_Service, &status)) {
		std::cout << "����װ�ɹ�: " << std::endl;
		std::cout << "��������: WIN32_OWN_PROCESS" << std::endl;
		std::cout << "��������: �Զ�����" << std::endl;
	} else {
		std::cout << "����װ�ɹ�����������֤ʧ��" << std::endl;
	}

	CloseService();
	return true;
}

bool ServiceManager::UninstallService() {
	if (!OpenService()) {
		std::wcout << L"���񲻴���: " << m_ServiceName << std::endl;
		return true;
	}

	SERVICE_STATUS status;
	if (ControlService(m_Service, SERVICE_CONTROL_STOP, &status)) {
		std::wcout << L"����ֹͣ����..." << std::endl;
		Sleep(1000);
	}

	if (!DeleteService(m_Service)) {
		CloseService();
		return false;
	}

	std::wcout << L"����ж�سɹ�: " << m_ServiceName << std::endl;
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
			std::wcout << L"������������: " << m_ServiceName << std::endl;
			return true;
		}
		return false;
	}

	std::wcout << L"���������ɹ�: " << m_ServiceName << std::endl;
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

	std::wcout << L"����ֹͣ�ɹ�: " << m_ServiceName << std::endl;
	CloseService();
	return true;
}

bool ServiceManager::QueryStatus() {
	if (!OpenService()) {
		std::wcout << L"����δ��װ: " << m_ServiceName << std::endl;
		return false;
	}

	SERVICE_STATUS status;
	if (!QueryServiceStatus(m_Service, &status)) {
		CloseService();
		return false;
	}

	std::wcout << L"����״̬: ";
	switch (status.dwCurrentState) {
	case SERVICE_STOPPED:
		std::wcout << L"��ֹͣ";
		break;
	case SERVICE_START_PENDING:
		std::wcout << L"������";
		break;
	case SERVICE_STOP_PENDING:
		std::wcout << L"ֹͣ��";
		break;
	case SERVICE_RUNNING:
		std::wcout << L"������";
		break;
	default:
		std::wcout << L"δ֪״̬";
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