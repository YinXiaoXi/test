@echo off
echo 检查Winlogon Manager Service状态...
echo.

echo 1. 检查服务是否存在
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo 服务不存在
    goto :end
) else (
    echo 服务存在
)
echo.

echo 2. 显示详细服务信息
sc query "WinlogonManagerService"
echo.

echo 3. 检查服务配置
sc qc "WinlogonManagerService"
echo.

echo 4. 尝试启动服务
sc start "WinlogonManagerService"
echo.

echo 5. 再次检查服务状态
sc query "WinlogonManagerService"
echo.

echo 6. 如果启动失败，检查事件日志
echo 检查系统事件日志中的错误...
wevtutil qe System /c:5 /rd:true /f:text /q:"*[System[Provider[@Name='Service Control Manager']]]" | findstr "WinlogonManagerService"
echo.

:end
echo 检查完成！
pause
