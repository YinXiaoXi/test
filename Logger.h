#pragma once

#include "Common.h"

class Logger {
public:
    static void Initialize();
    static void Shutdown();
    
    static void Log(LogLevel level, const String& message);
    static void Log(LogLevel level, const WString& message);
    static void Log(LogLevel level, const char* format, ...);
    static void Log(LogLevel level, const wchar_t* format, ...);
    
    static void SetLogLevel(LogLevel level);
    static void SetLogToFile(bool enable, const String& filePath = "");
    
private:
    static std::mutex s_logMutex;
    static LogLevel s_currentLevel;
    static bool s_logToFile;
    static String s_logFilePath;
    static HANDLE s_logFile;
    
    static void WriteLog(LogLevel level, const String& message);
    static String GetLogLevelString(LogLevel level);
    static String GetCurrentTimeString();
};