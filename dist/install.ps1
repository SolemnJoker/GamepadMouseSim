# 手柄鼠标模拟器 安装程序
# 用法：右键以管理员身份运行 install.ps1

param(
    [switch]$AutoStart,
    [switch]$Silent
)

$ErrorActionPreference = "Stop"
$AppName = "GamepadMouseSim"
$AppVersion = "1.0.0"
$InstallDir = "$env:LOCALAPPDATA\$AppName"
$ExePath = "$InstallDir\GamepadMouseSim.exe"
$RegRunKey = "HKCU\Software\Microsoft\Windows\CurrentVersion\Run"

function Write-Log {
    param([string]$Message)
    $ts = Get-Date -Format "HH:mm:ss"
    Write-Host "[$ts] $Message" -ForegroundColor Cyan
}

function Test-Admin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]$identity
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "  手柄鼠标模拟器 v$AppVersion" -ForegroundColor Yellow
Write-Host "  Gamepad Mouse Simulator" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Write-Host ""

if (Test-Admin) {
    Write-Log "检测到管理员权限"
} else {
    Write-Log "警告：未使用管理员权限，部分功能可能受限"
}

# 停止运行中的程序
Write-Log "停止运行中的程序..."
Get-Process $AppName -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# 创建安装目录
Write-Log "创建安装目录: $InstallDir"
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
New-Item -ItemType Directory -Force -Path "$InstallDir\config" | Out-Null

# 复制文件
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Write-Log "复制程序文件..."
Copy-Item "$ScriptDir\GamepadMouseSim.exe" "$InstallDir\" -Force
Copy-Item "$ScriptDir\*.dll" "$InstallDir\" -Force

Write-Log "复制插件..."
foreach ($subdir in @("platforms","styles","imageformats","iconengines","generic","tls","networkinformation")) {
    if (Test-Path "$ScriptDir\$subdir") {
        New-Item -ItemType Directory -Force -Path "$InstallDir\$subdir" | Out-Null
        Copy-Item "$ScriptDir\$subdir\*.dll" "$InstallDir\$subdir\" -Force -ErrorAction SilentlyContinue
    }
}

# 复制配置文件（不覆盖已有配置）
Write-Log "检查配置文件..."
if (-not (Test-Path "$InstallDir\config.json")) {
    Write-Log "创建默认配置文件..."
    if (Test-Path "$ScriptDir\config\default_config.json") {
        Copy-Item "$ScriptDir\config\default_config.json" "$InstallDir\config.json" -Force
    }
} else {
    Write-Log "检测到已有配置文件，保留不变"
}

# 创建桌面快捷方式
Write-Log "创建桌面快捷方式..."
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\手柄鼠标模拟器.lnk")
$Shortcut.TargetPath = $ExePath
$Shortcut.WorkingDirectory = $InstallDir
$Shortcut.Description = "手柄鼠标模拟器 - Gamepad Mouse Simulator"
$Shortcut.Save()

# 开机启动设置
Write-Log "设置开机启动..."
try {
    if ($AutoStart) {
        Set-ItemProperty -Path $RegRunKey -Name $AppName -Value "`"$ExePath`"" -Force
        Write-Log "已启用开机启动"
    } else {
        if (Test-Path "$RegRunKey\$AppName") {
            Remove-ItemProperty -Path $RegRunKey -Name $AppName -Force
            Write-Log "已禁用开机启动"
        }
    }
} catch {
    Write-Log "警告：设置开机启动失败（需要管理员权限）"
}

# 创建卸载脚本
Write-Log "创建卸载脚本..."
@"
# 手柄鼠标模拟器 卸载程序
`$AppName = "GamepadMouseSim"
`$InstallDir = "`$env:LOCALAPPDATA\`$AppName"
`$RegRunKey = "HKCU\Software\Microsoft\Windows\CurrentVersion\Run"

Write-Host "正在卸载 `$AppName..." -ForegroundColor Yellow
Get-Process `$AppName -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

Remove-Item "`$InstallDir" -Recurse -Force -ErrorAction SilentlyContinue
Remove-ItemProperty -Path `$RegRunKey -Name `$AppName -Force -ErrorAction SilentlyContinue
Remove-Item "`$env:USERPROFILE\Desktop\手柄鼠标模拟器.lnk" -Force -ErrorAction SilentlyContinue

Write-Host "`$AppName 已卸载" -ForegroundColor Green
"@ | Set-Content "$InstallDir\uninstall.ps1" -Encoding UTF8

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  安装完成！" -ForegroundColor Green
Write-Host "  安装位置: $InstallDir" -ForegroundColor Green
Write-Host "  桌面快捷方式: 已创建" -ForegroundColor Green
if ($AutoStart) {
    Write-Host "  开机启动: 已启用" -ForegroundColor Green
}
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "运行: $ExePath" -ForegroundColor White
Write-Host "卸载: $InstallDir\uninstall.ps1" -ForegroundColor Gray
Write-Host ""
