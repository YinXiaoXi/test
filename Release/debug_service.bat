@echo off
echo ����Winlogon Manager Service...
echo.

echo 1. ��鵱ǰ�û�Ȩ��...
whoami
echo.

echo 2. ��������ƹ���������Ȩ��...
net session >nul 2>&1
if %errorlevel% == 0 (
    echo ��ǰ�û����й���ԱȨ��
) else (
    echo ��ǰ�û�û�й���ԱȨ�ޣ����Թ���Ա������д˽ű�
    echo.
    pause
    exit /b 1
)
echo.

echo 3. �������Ƿ��Ѱ�װ...
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo �����Ѱ�װ
    sc query "WinlogonManagerService"
) else (
    echo ����δ��װ
)
echo.

echo 4. ���԰�װ����...
service.exe --install
echo.

echo 5. ��鰲װ��ķ���״̬...
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo ����װ�ɹ�
    sc query "WinlogonManagerService"
) else (
    echo ����װʧ��
)
echo.

echo 6. ������������...
service.exe --start
echo.

echo 7. ����������״̬...
sc query "WinlogonManagerService"
echo.

echo ������ɣ�
pause

