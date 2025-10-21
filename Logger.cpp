#include "Logger.h"
#include <fstream>
#include <chrono>
#include <iomanip>

std::mutex Logger::s_logMutex;
LogLevel Logger::s_currentLevel = LogLevel::Info;
bool Logger::s_logToFile = false;
String Logger::s_logFilePath;
HANDLE Logger::s_logFile = INVALID_HANDLE_VALUE;

void Logger::Initialize() {
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_currentLevel = LogLevel::Info;
    s_logToFile = false;
    s_logFile = INVALID_HANDLE_VALUE;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(s_logMutex);
    if (s_logFile != INVALID_HANDLE_VALUE) {
        CloseHandle(s_logFile);
        s_logFile = INVALID_HANDLE_VALUE;
    }
}

void Logger::Log(LogLevel level, const String& message) {
    if (level < s_currentLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(s_logMutex);
    WriteLog(level, message);
}

void Logger::Log(LogLevel level, const WString& message) {
    Log(level, Utils::WStringToString(message));
}

void Logger::Log(LogLevel level, const char* format, ...) {
    if (level < s_currentLevel) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    
    va_end(args);
    
    Log(level, String(buffer));
}

void Logger::Log(LogLevel level, const wchar_t* format, ...) {
    if (level < s_currentLevel) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    wchar_t buffer[1024];
    vswprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    
    va_end(args);
    
    Log(level, WString(buffer));
}

void Logger::SetLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_currentLevel = level;
}

void Logger::SetLogToFile(bool enable, const String& filePath) {
    std::lock_guard<std::mutex> lock(s_logMutex);
    
    if (s_logFile != INVALID_HANDLE_VALUE) {
        CloseHandle(s_logFile);
        s_logFile = INVALID_HANDLE_VALUE;
    }
    
    s_logToFile = enable;
    s_logFilePath = filePath;
    
    if (enable && !filePath.empty()) {
        s_logFile = CreateFileA(
            filePath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (s_logFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(s_logFile, 0, NULL, FILE_END);
        }
    }
}

void Logger::WriteLog(LogLevel level, const String& message) {
    String logMessage = GetCurrentTimeString() + " [" + GetLogLevelString(level) + "] " + message + "\n";
    
    // 输出到控制台
    std::cout << logMessage;
    
    // 输出到文件
    if (s_logToFile && s_logFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(s_logFile, logMessage.c_str(), static_cast<DWORD>(logMessage.length()), &bytesWritten, NULL);
        FlushFileBuffers(s_logFile);
    }
}

String Logger::GetLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

String Logger::GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}