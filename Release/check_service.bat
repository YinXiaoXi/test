@echo off
echo ���Winlogon Manager Service״̬...
echo.

echo 1. �������Ƿ����
sc query "WinlogonManagerService" >nul 2>&1
if errorlevel 1 (
    echo ���񲻴���
    goto :end
) else (
    echo �������
)
echo.

echo 2. ��ʾ��ϸ������Ϣ
sc query "WinlogonManagerService"
echo.

echo 3. ����������
sc qc "WinlogonManagerService"
echo.

echo 4. ������������
sc start "WinlogonManagerService"
echo.

echo 5. �ٴμ�����״̬
sc query "WinlogonManagerService"
echo.

echo 6. �������ʧ�ܣ�����¼���־
echo ���ϵͳ�¼���־�еĴ���...
wevtutil qe System /c:5 /rd:true /f:text /q:"*[System[Provider[@Name='Service Control Manager']]]" | findstr "WinlogonManagerService"
echo.

:end
echo �����ɣ�
pause
