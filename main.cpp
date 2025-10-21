#include "WinlogonService.h"

int main(int argc, char* argv[]) {
    try {
        WinlogonService service;
        return service.Run(argc, argv);
    }
    catch (const std::exception& e) {
        Logger::Log(LogLevel::Error, "Exception caught: %s", e.what());
        return 1;
    }
    catch (...) {
        Logger::Log(LogLevel::Error, "Unknown exception caught");
        return 1;
    }
}