@echo off
echo ����Winlogon Manager Service...
echo.

echo 1. ֹͣ��������������У�
sc stop "WinlogonManagerService" >nul 2>&1
echo.

echo 2. ɾ������
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo 3. �ȴ�������ȫɾ��
timeout /t 3 /nobreak >nul
echo.

echo 4. �������Ƿ���ɾ��
sc query "WinlogonManagerService" >nul 2>&1
if %errorlevel% == 0 (
    echo ������Ȼ���ڣ����Ե�Ƭ�̺�����
) else (
    echo �����ѳɹ�ɾ��
)
echo.

echo ������ɣ�
pause

