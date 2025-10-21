#pragma once

#include <windows.h>
#include <string>

class SingleInstance {
public:
	SingleInstance(const char* mutexName);
	~SingleInstance();

	bool IsFirstInstance();

private:
	HANDLE m_Mutex;
	std::string m_MutexName;
	bool m_IsFirstInstance;
};