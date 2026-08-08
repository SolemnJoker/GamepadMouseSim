# 手柄鼠标模拟器 (Gamepad Mouse Simulator)

Windows 平台手柄鼠标模拟工具，将 Xbox 手柄按键映射为鼠标/键盘操作，支持多手柄同时使用。

## ✨ 特性

- **多手柄支持** — 最多 4 个手柄同时使用，每个手柄独立切换模式
- **手动模式切换** — 长按 LT+View 1秒切换鼠标/默认模式，无需依赖进程检测
- **LT 修饰层** — 按住 LT 扳机进入 Alt 组合键模式，实现窗口切换等操作
- **RT 修饰层** — 按住 RT 扳机进入 Ctrl 组合键模式，实现标签切换、复制粘贴等
- **摇杆控制** — 左摇杆移动鼠标光标（含加速曲线），右摇杆控制滚轮
- **音量控制** — Dpad 十字键控制音量增大/减小/静音
- **帮助屏幕** — LT+R3 在全屏 85% 区域显示所有按键操作说明（中文）
- **系统托盘** — 最小化到托盘运行，显示各手柄模式状态
- **OSD 通知** — 模式切换时屏幕右下角弹出半透明提示
- **配置热加载** — JSON 配置文件，修改后自动生效无需重启
- **安装包** — Inno Setup 制作安装包，一键安装

## 🚀 快速开始

### 环境要求

- Windows 10/11 64位
- Xbox 手柄（兼容 XInput 的第三方手柄）
- Qt 6.11.1 + MinGW 13.1.0

### 编译

```bash
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j 8
```

### 运行

```bash
.\GamepadMouseSim.exe
```

程序启动后最小化到系统托盘（右下角），默认进入默认模式（游戏手柄原样直通，长按 LT+View 1 秒切换为鼠标模式）。

### 安装包

使用 Inno Setup 6 编译安装脚本：

```bash
cd installer
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" GamepadMouseSim.iss
```

## 🎮 基本操作

| 按键 | 功能 |
|------|------|
| A | 鼠标左键 |
| B | 鼠标右键 |
| X | 鼠标中键 |
| LB | 按住左键（配合摇杆拖拽） |
| RB | 按住右键（配合摇杆拖拽） |
| Y | 回车 |
| L3 | 回车 |
| R3 | 退出 |
| View | Tab |
| Menu | 开始菜单 |

<sup>按键映射来源: core/Types.cpp actionToChinese()</sup>

### 扳机组和层 (LT/RT)

**按住 LT +**

| 组合 | 功能 |
|------|------|
| LT+X | Alt+Tab |
| LT+LB/RB | KeyShiftTab / Tab |
| LT+Y | 关闭窗口 |
| LT+R3 | 显示帮助 |
| LT+View |  |

<sup>LT/RT 层修改器在 InputMapper.cpp 中定义</sup>
## 📖 文档

- [使用说明书](USER_MANUAL.md) — 详细操作指南和常见问题
- [实现总结](IMPLEMENTATION_SUMMARY.md) — 架构设计、技术方案
- [测试指南](TEST_GUIDE.md) — 功能测试方法
- [测试表](TEST_TABLE.md) — 功能测试清单
- [需求规格](readme.md) — 原始需求文档

## ⚙️ 配置

程序首次运行在 `config.json` 生成默认配置，修改后自动生效。

```json
{
  "monitoring": {
    "manual_switch_lockout_seconds": 3
  },
  "combo_key": {
    "buttons": ["LT", "View"],
    "hold_duration_ms": 1000
  },
  "mouse_mode": {
    "left_stick": { "sensitivity_x": 1.0, "deadzone": 0.15 }
  }
}
```

## 📝 License

MIT
