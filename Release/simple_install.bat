@echo off
chcp 65001 >nul
echo 简化安装Winlogon Manager Service...
echo.

echo 步骤1: 卸载旧服务（如果存在）
service.exe --uninstall
echo.

echo 步骤2: 安装新服务
service.exe --install
echo.

echo 步骤3: 检查安装结果
if %errorlevel% == 0 (
    echo 服务安装成功！
    echo.
    echo 步骤4: 启动服务
    service.exe --start
    echo.
    echo 步骤5: 检查服务状态
    service.exe --status
) else (
    echo 服务安装失败，请检查错误信息
)

echo.
echo 安装过程完成！
pause

