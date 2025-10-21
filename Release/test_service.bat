@echo off
echo 测试Winlogon Manager Service...
echo.

echo 1. 检查服务状态...
service.exe --status
echo.

echo 2. 测试挂起winlogon进程...
service.exe --suspend
echo.

timeout /t 3 /nobreak >nul

echo 3. 测试恢复winlogon进程...
service.exe --resume
echo.

echo 4. 再次检查服务状态...
service.exe --status
echo.

echo 测试完成！
pause
