@echo off
echo ���ڰ�װWinlogon Manager Service...
echo.

REM ж�ؾɷ���������ڣ�
echo ж�ؾɷ���...
service.exe --uninstall
echo.

REM ��װ�·���
echo ��װ�·���...
service.exe --install
echo.

REM ��������
echo ��������...
service.exe --start
echo.

REM ������״̬
echo ������״̬...
service.exe --status
echo.

echo ��װ��ɣ�
pause
