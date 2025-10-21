@echo off
echo 修复Winlogon Manager Service配置...
echo.

echo 步骤1: 完全删除损坏的服务
sc stop "WinlogonManagerService" >nul 2>&1
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo 步骤2: 清理所有注册表项
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet001\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet002\Services\WinlogonManagerService" /f >nul 2>&1
echo 注册表清理完成
echo.

echo 步骤3: 等待系统完全清理
timeout /t 15 /nobreak >nul
echo.

echo 步骤4: 验证服务是否完全删除
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo 服务已完全删除
) else (
    echo 警告: 服务仍然存在，需要重启系统
    echo 请重启系统后再次运行此脚本
    pause
    exit /b 1
)
echo.

echo 步骤5: 手动创建服务配置
echo 创建服务注册表项...
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "Type" /t REG_DWORD /d 16 /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "Start" /t REG_DWORD /d 2 /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "ErrorControl" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "ImagePath" /t REG_SZ /d "D:\ud\Desktop\WindowsService\Release\service.exe" /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "DisplayName" /t REG_SZ /d "Winlogon Manager Service" /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "Description" /t REG_SZ /d "管理winlogon进程的服务" /f >nul
echo 服务配置创建完成
echo.

echo 步骤6: 验证服务配置
sc qc "WinlogonManagerService"
echo.

echo 步骤7: 尝试启动服务
sc start "WinlogonManagerService"
echo.

echo 步骤8: 检查最终状态
sc query "WinlogonManagerService"
echo.

echo 配置修复完成！
pause
