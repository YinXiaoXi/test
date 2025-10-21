@echo off
echo 强制清理Winlogon Manager Service...
echo.

echo 步骤1: 强制停止服务
sc stop "WinlogonManagerService" >nul 2>&1
net stop "WinlogonManagerService" >nul 2>&1
echo.

echo 步骤2: 强制删除服务
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo 步骤3: 等待并检查删除状态
for /L %%i in (1,1,10) do (
    echo 等待删除... %%i/10
    timeout /t 2 /nobreak >nul
    sc query "WinlogonManagerService" >nul 2>&1
    if errorlevel 1 (
        echo 服务已成功删除！
        goto :install
    )
)
echo 服务删除超时，尝试其他方法...
echo.

echo 步骤4: 使用注册表清理（需要管理员权限）
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /f >nul 2>&1
echo 注册表清理完成
echo.

:install
echo 步骤5: 重新安装服务
service.exe --install
echo.

echo 步骤6: 检查最终状态
sc query "WinlogonManagerService"
echo.

echo 强制清理完成！
pause
