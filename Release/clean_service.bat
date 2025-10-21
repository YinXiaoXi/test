@echo off
echo 清理Winlogon Manager Service...
echo.

echo 1. 停止服务（如果正在运行）
sc stop "WinlogonManagerService" >nul 2>&1
echo.

echo 2. 删除服务
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo 3. 等待服务完全删除
timeout /t 3 /nobreak >nul
echo.

echo 4. 检查服务是否已删除
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo 服务仍然存在，请稍等片刻后重试
) else (
    echo 服务已成功删除
)
echo.

echo 清理完成！
pause

