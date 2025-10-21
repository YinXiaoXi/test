@echo off
echo 终极修复Winlogon Manager Service...
echo.

echo 步骤1: 完全停止所有相关进程
taskkill /f /im service.exe >nul 2>&1
echo.

echo 步骤2: 强制停止服务
sc stop "WinlogonManagerService" >nul 2>&1
net stop "WinlogonManagerService" >nul 2>&1
echo.

echo 步骤3: 强制删除服务
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo 步骤4: 清理注册表
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet001\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet002\Services\WinlogonManagerService" /f >nul 2>&1
echo 注册表清理完成
echo.

echo 步骤5: 等待系统清理
echo 等待系统完全清理服务...
timeout /t 10 /nobreak >nul
echo.

echo 步骤6: 验证服务是否已删除
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo 服务已成功删除
) else (
    echo 警告: 服务仍然存在，可能需要重启系统
    echo 继续尝试安装...
)
echo.

echo 步骤7: 重新安装服务
service.exe --install
echo.

echo 步骤8: 验证安装结果
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo 服务安装失败
) else (
    echo 服务安装成功
    echo.
    echo 步骤9: 启动服务
    sc start "WinlogonManagerService"
    echo.
    echo 步骤10: 检查最终状态
    sc query "WinlogonManagerService"
)

echo.
echo 终极修复完成！
echo 如果服务仍然无法启动，请重启系统后重试
pause

