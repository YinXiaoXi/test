@echo off
echo �޸�Winlogon Manager Service����...
echo.

echo ����1: ��ȫɾ���𻵵ķ���
sc stop "WinlogonManagerService" >nul 2>&1
sc delete "WinlogonManagerService" >nul 2>&1
echo.

echo ����2: ��������ע�����
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet001\Services\WinlogonManagerService" /f >nul 2>&1
reg delete "HKLM\SYSTEM\ControlSet002\Services\WinlogonManagerService" /f >nul 2>&1
echo ע����������
echo.

echo ����3: �ȴ�ϵͳ��ȫ����
timeout /t 15 /nobreak >nul
echo.

echo ����4: ��֤�����Ƿ���ȫɾ��
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo ��������ȫɾ��
) else (
    echo ����: ������Ȼ���ڣ���Ҫ����ϵͳ
    echo ������ϵͳ���ٴ����д˽ű�
    pause
    exit /b 1
)
echo.

echo ����5: �ֶ�������������
echo ��������ע�����...
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "Type" /t REG_DWORD /d 16 /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "Start" /t REG_DWORD /d 2 /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "ErrorControl" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "ImagePath" /t REG_SZ /d "D:\ud\Desktop\WindowsService\Release\service.exe" /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "DisplayName" /t REG_SZ /d "Winlogon Manager Service" /f >nul
reg add "HKLM\SYSTEM\CurrentControlSet\Services\WinlogonManagerService" /v "Description" /t REG_SZ /d "����winlogon���̵ķ���" /f >nul
echo �������ô������
echo.

echo ����6: ��֤��������
sc qc "WinlogonManagerService"
echo.

echo ����7: ������������
sc start "WinlogonManagerService"
echo.

echo ����8: �������״̬
sc query "WinlogonManagerService"
echo.

echo �����޸���ɣ�
pause
