@echo off
echo �޸�Winlogon Manager Service...
echo.

echo ����1: ��ȫ����ɷ���
echo ֹͣ����...
sc stop "WinlogonManagerService" >nul 2>&1
echo ɾ������...
sc delete "WinlogonManagerService" >nul 2>&1
echo �ȴ�������ȫɾ��...
timeout /t 5 /nobreak >nul
echo.

echo ����2: �������Ƿ���ɾ��
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo ����: ������Ȼ���ڣ�������Ҫ����ϵͳ
    echo �������԰�װ...
) else (
    echo �����ѳɹ�ɾ��
)
echo.

echo ����3: ��װ�·���
service.exe --install
echo.

echo ����4: ��鰲װ���
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo ����װ�ɹ���
    echo.
    echo ����5: ��������
    service.exe --start
    echo.
    echo ����6: ������״̬
    service.exe --status
) else (
    echo ����װʧ�ܣ����������Ϣ
)

echo.
echo �޸���ɣ�
pause

