@echo off
echo ����Winlogon Manager Service...
echo.

echo 1. ������״̬...
service.exe --status
echo.

echo 2. ���Թ���winlogon����...
service.exe --suspend
echo.

timeout /t 3 /nobreak >nul

echo 3. ���Իָ�winlogon����...
service.exe --resume
echo.

echo 4. �ٴμ�����״̬...
service.exe --status
echo.

echo ������ɣ�
pause
