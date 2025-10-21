@echo off
echo 重启后安装Winlogon Manager Service...
echo.

echo 步骤1: 检查系统是否已重启
echo 如果看到此消息，说明系统已重启
echo.

echo 步骤2: 清理任何残留的服务
sc stop "WinlogonManagerService" >nul 2>&1
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo 步骤3: 等待清理完成
timeout /t 5 /nobreak >nul
echo.

echo 步骤4: 使用程序安装服务
service.exe --install
echo.

echo 步骤5: 检查安装结果
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo 服务安装失败
    echo 尝试手动配置...
    call fix_service_config.bat
) else (
    echo 服务安装成功
    echo.
    echo 步骤6: 启动服务
    sc start "WinlogonManagerService"
    echo.
    echo 步骤7: 检查服务状态
    sc query "WinlogonManagerService"
)

echo.
echo 重启后安装完成！
pause
