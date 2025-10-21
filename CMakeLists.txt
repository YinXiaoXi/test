cmake_minimum_required(VERSION 3.16)
project(WinlogonManagerService VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 源文件
set(SOURCES
    main.cpp
    WinlogonService.cpp
    ServiceManager.cpp
    ProcessManager.cpp
    IPCManager.cpp
    Logger.cpp
    Utils.cpp
)

# 头文件
set(HEADERS
    Common.h
    WinlogonService.h
    ServiceManager.h
    ProcessManager.h
    IPCManager.h
    Logger.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "service"
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    advapi32
    kernel32
    user32
    psapi
    tlhelp32
)

# 编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# 编译选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /WX-
        /permissive-
        /Zc:__cplusplus
    )
    
    # 设置字符集
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    
    # 设置运行时库
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 创建安装脚本
if(WIN32)
    configure_file(
        ${CMAKE_SOURCE_DIR}/install_service.bat.in
        ${CMAKE_BINARY_DIR}/install_service.bat
        @ONLY
    )
    
    configure_file(
        ${CMAKE_SOURCE_DIR}/uninstall_service.bat.in
        ${CMAKE_BINARY_DIR}/uninstall_service.bat
        @ONLY
    )
endif()