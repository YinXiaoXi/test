@echo off
echo �ռ��޸�Winlogon Manager Service...
echo.

echo ����1: ��ȫֹͣ������ؽ���
taskkill /f /im service.exe >nul 2>&1
echo.

echo ����2: ǿ��ֹͣ����
sc stop "WinlogonManagerService" >nul 2>&1
net stop "WinlogonManagerService" >nul 2>&1
echo.

echo ����3: ǿ��ɾ������
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo ����4: ����ע���
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet001\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet002\Services\WinlogonManagerService" /f >nul 2>&1
echo ע����������
echo.

echo ����5: �ȴ�ϵͳ����
echo �ȴ�ϵͳ��ȫ�������...
timeout /t 10 /nobreak >nul
echo.

echo ����6: ��֤�����Ƿ���ɾ��
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo �����ѳɹ�ɾ��
) else (
    echo ����: ������Ȼ���ڣ�������Ҫ����ϵͳ
    echo �������԰�װ...
)
echo.

echo ����7: ���°�װ����
service.exe --install
echo.

echo ����8: ��֤��װ���
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo ����װʧ��
) else (
    echo ����װ�ɹ�
    echo.
    echo ����9: ��������
    sc start "WinlogonManagerService"
    echo.
    echo ����10: �������״̬
    sc query "WinlogonManagerService"
)

echo.
echo �ռ��޸���ɣ�
echo ���������Ȼ�޷�������������ϵͳ������
pause

