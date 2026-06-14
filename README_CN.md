# 手柄鼠标模拟器 (Gamepad Mouse Simulator)

Windows 平台手柄鼠标模拟工具，将 Xbox 手柄按键映射为鼠标/键盘操作，让你用手柄也能轻松控制桌面。

## ✨ 特性

- **智能模式切换** — 自动检测游戏进程，游戏运行时手柄原样透传，桌面时映射为鼠标键盘
- **LT 修饰层** — 按住 LT 扳机进入 Alt 组合键模式，实现窗口切换、关闭窗口等操作
- **RT 修饰层** — 按住 RT 扳机进入 Ctrl 组合键模式，实现标签切换、复制粘贴、翻页等操作
- **摇杆控制** — 左摇杆移动鼠标光标（含加速曲线），右摇杆控制滚轮
- **音量控制** — Dpad 十字键控制音量增大/减小/静音
- **系统托盘** — 最小化到托盘运行，图标实时反映当前模式
- **OSD 通知** — 模式切换时屏幕右下角弹出半透明提示
- **配置热加载** — JSON 配置文件，修改后自动生效无需重启
- **低资源占用** — 后台 CPU <1%，内存 <50MB

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

程序启动后最小化到系统托盘（右下角）。

## 🎮 基本操作

| 按键 | 功能 |
|------|------|
| A / B / X | 鼠标左键 / 右键 / 中键 |
| LB / RB | 按住左键 / 右键（配合左摇杆拖拽） |
| Y | 回车(Enter) |
| L3 / R3 | 回车(Enter) / 退出(Escape) |
| View / Menu | Tab键 / Win键 |
| 左摇杆 | 移动鼠标光标 |
| 右摇杆 | 滚轮 |
| Dpad ↑↓← | 音量增减 / 静音 |
| **按住 LT +** | **Alt 组合键模式** |
| LT+X | 打开窗口切换器(Alt+Tab) |
| LT+LB/RB | 切换器左/右选择 |
| LT+Y | 关闭窗口(Alt+F4) |
| **按住 RT +** | **Ctrl 组合键模式** |
| RT+Dpad→/← | 浏览器标签切换(Ctrl+Tab) |
| RT+Dpad↑/↓ | 翻页(PageUp/Down) |
| RT+A/B/X/Y | 全选/复制/剪切/粘贴 |
| View+Menu 按住1秒 | 切换鼠标模式↔默认模式 |

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
    "process_list": ["steam.exe"],
    "polling_interval_seconds": 2
  },
  "mouse_mode": {
    "left_stick": { "sensitivity_x": 1.0, "deadzone": 0.15 }
  }
}
```

## 📝 License

MIT