@echo off
echo ������װWinlogon Manager Service...
echo.

echo ����1: ���ϵͳ�Ƿ�������
echo �����������Ϣ��˵��ϵͳ������
echo.

echo ����2: �����κβ����ķ���
sc stop "WinlogonManagerService" >nul 2>&1
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo ����3: �ȴ��������
timeout /t 5 /nobreak >nul
echo.

echo ����4: ʹ�ó���װ����
service.exe --install
echo.

echo ����5: ��鰲װ���
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo ����װʧ��
    echo �����ֶ�����...
    call fix_service_config.bat
) else (
    echo ����װ�ɹ�
    echo.
    echo ����6: ��������
    sc start "WinlogonManagerService"
    echo.
    echo ����7: ������״̬
    sc query "WinlogonManagerService"
)

echo.
echo ������װ��ɣ�
pause
