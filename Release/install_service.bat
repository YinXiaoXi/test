@echo off
echo 正在安装Winlogon Manager Service...
echo.

REM 卸载旧服务（如果存在）
echo 卸载旧服务...
service.exe --uninstall
echo.

REM 安装新服务
echo 安装新服务...
service.exe --install
echo.

REM 启动服务
echo 启动服务...
service.exe --start
echo.

REM 检查服务状态
echo 检查服务状态...
service.exe --status
echo.

echo 安装完成！
pause
