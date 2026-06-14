@echo off
chcp 65001 >nul
echo ========================================
echo   手柄鼠标模拟器 安装程序
echo ========================================
echo.
echo 选项:
echo   1. 安装（默认）
echo   2. 安装并启用开机启动
echo   3. 卸载
echo.
set /p choice="请输入选项 (1/2/3): "

if "%choice%"=="1" (
    powershell -ExecutionPolicy Bypass -File "%~dp0install.ps1"
) else if "%choice%"=="2" (
    powershell -ExecutionPolicy Bypass -File "%~dp0install.ps1" -AutoStart
) else if "%choice%"=="3" (
    if exist "%LOCALAPPDATA%\GamepadMouseSim\uninstall.ps1" (
        powershell -ExecutionPolicy Bypass -File "%LOCALAPPDATA%\GamepadMouseSim\uninstall.ps1"
    ) else (
        echo 未找到卸载程序，请手动删除 %LOCALAPPDATA%\GamepadMouseSim
    )
) else (
    echo 无效选项
)

pause
