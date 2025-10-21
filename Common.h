#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <sstream>

// 常用类型定义
using String = std::string;
using WString = std::wstring;
using CommandHandler = std::function<bool(const String&)>;

// 错误代码定义
enum class ErrorCode {
    Success = 0,
    InvalidParameter = 1,
    AccessDenied = 2,
    ServiceNotFound = 3,
    ProcessNotFound = 4,
    IPCConnectionFailed = 5,
    UnknownError = 999
};

// 服务状态定义
enum class ServiceState {
    Stopped = 0,
    Starting = 1,
    Running = 2,
    Stopping = 3
};

// 进程信息结构
struct ProcessInfo {
    DWORD processId;
    WString processName;
    DWORD threadCount;
    DWORD parentProcessId;
    
    ProcessInfo() : processId(0), threadCount(0), parentProcessId(0) {}
};

// 日志级别
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

// 日志工具类
class Logger {
public:
    static void Log(LogLevel level, const String& message);
    static void Log(LogLevel level, const WString& message);
    
private:
    static std::mutex s_logMutex;
    static void WriteLog(LogLevel level, const String& message);
};

// 工具函数
class Utils {
public:
    static WString StringToWString(const String& str);
    static String WStringToString(const WString& str);
    static String GetLastErrorString();
    static bool IsRunningAsAdmin();
    static String GetModulePath();
};