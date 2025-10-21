#pragma once

#include "Common.h"

class Utils {
public:
    static WString StringToWString(const String& str);
    static String WStringToString(const WString& str);
    static String GetLastErrorString();
    static bool IsRunningAsAdmin();
    static String GetModulePath();
};