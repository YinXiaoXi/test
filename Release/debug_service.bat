@echo off
echo 调试Winlogon Manager Service...
echo.

echo 1. 检查当前用户权限...
whoami
echo.

echo 2. 检查服务控制管理器访问权限...
net session >nul 2>&1
if %errorlevel% == 0 (
    echo 当前用户具有管理员权限
) else (
    echo 当前用户没有管理员权限，请以管理员身份运行此脚本
    echo.
    pause
    exit /b 1
)
echo.

echo 3. 检查服务是否已安装...
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo 服务已安装
    sc query "WinlogonManagerService"
) else (
    echo 服务未安装
)
echo.

echo 4. 尝试安装服务...
service.exe --install
echo.

echo 5. 检查安装后的服务状态...
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo 服务安装成功
    sc query "WinlogonManagerService"
) else (
    echo 服务安装失败
)
echo.

echo 6. 尝试启动服务...
service.exe --start
echo.

echo 7. 检查服务运行状态...
sc query "WinlogonManagerService"
echo.

echo 调试完成！
pause

