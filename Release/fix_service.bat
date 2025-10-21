@echo off
echo 修复Winlogon Manager Service...
echo.

echo 步骤1: 完全清理旧服务
echo 停止服务...
sc stop "WinlogonManagerService" >nul 2>&1
echo 删除服务...
sc delete "WinlogonManagerService" >nul 2>&1
echo 等待服务完全删除...
timeout /t 5 /nobreak >nul
echo.

echo 步骤2: 检查服务是否已删除
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo 警告: 服务仍然存在，可能需要重启系统
    echo 继续尝试安装...
) else (
    echo 服务已成功删除
)
echo.

echo 步骤3: 安装新服务
service.exe --install
echo.

echo 步骤4: 检查安装结果
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo 服务安装成功！
    echo.
    echo 步骤5: 启动服务
    service.exe --start
    echo.
    echo 步骤6: 检查服务状态
    service.exe --status
) else (
    echo 服务安装失败，请检查错误信息
)

echo.
echo 修复完成！
pause

