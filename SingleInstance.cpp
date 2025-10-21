#include "SingleInstance.h"

SingleInstance::SingleInstance(const char* mutexName) 
: m_Mutex(NULL)
, m_MutexName(mutexName) {

	m_Mutex = CreateMutexA(NULL, TRUE, mutexName);
	m_IsFirstInstance = (m_Mutex != NULL) && (GetLastError() != ERROR_ALREADY_EXISTS);
}

SingleInstance::~SingleInstance() {
	if (m_Mutex) {
		CloseHandle(m_Mutex);
	}
}

bool SingleInstance::IsFirstInstance() {
	return m_IsFirstInstance;
}