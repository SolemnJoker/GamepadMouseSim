# 手柄鼠标模拟器 - 使用说明书

## 快速开始

1. 插入Xbox手柄（兼容XInput的第三方手柄也可，最多4个同时使用）
2. 运行 `GamepadMouseSim.exe`
3. 程序自动最小化到**系统托盘**（右下角图标区）
4. 程序默认进入**鼠标模式**，手柄按键被映射为鼠标/键盘操作
5. **按住LT+View 1秒**切换鼠标↔默认模式

> **提示**：启动时如果Windows弹出安全警告，点击"更多信息"→"仍要运行"即可

---

## 两种工作模式

| 模式 | 什么时候用 | 手柄行为 | 托盘图标 |
|------|-----------|---------|--------|
| **鼠标模式** | 桌面操作、浏览网页、编辑文档 | 手柄按键映射为鼠标/键盘 | 🖱 鼠标图标 |
| **默认模式** | 玩游戏 | 手柄信号原样传给游戏 | 🎮 手柄图标 |

### 切换模式

**长按 LT+View 1秒**切换模式。每个手柄独立切换，互不影响。锁定期3秒防止误触。

---

## 多手柄支持

- 最多连接**4个手柄**同时使用
- 每个手柄**独立**管理自己的模式状态
- 托盘菜单显示每个手柄的模式：`P1:M  P2:D`（M=鼠标模式，D=默认模式）
- OSD通知会标明是哪个手柄切换了模式：`Pad1: Mouse`

---

## 帮助屏幕

**按住LT + 按R3** 显示全屏操作说明（覆盖屏幕 85%区域），中文显示，多列自动布局，5秒后自动消失。

---

## 按键操作指南

### 基础操作

| 手柄按键 | 效果 | 常用场景 |
|---------|------|---------|
| A | 鼠标左键单击 | 点击按钮、链接 |
| B | 鼠标右键单击 | 弹出右键菜单 |
| X | 鼠标中键单击 | 关闭浏览器标签页 |
| Y | 回车键(Enter) | 确认 |
| LB | 按住鼠标左键 | 配合左摇杆拖拽文件 |
| RB | 按住鼠标右键 | 右键拖拽 |
| L3（按下左摇杆）| 回车(Enter) | 确认 |
| R3（按下右摇杆）| 退出(Escape) | 关闭弹窗、取消 |
| View | Tab键 | 切换焦点 |
| Menu | Win键 | 打开开始菜单 |
| 左摇杆 | 移动鼠标光标 | 轻推慢速、重推快速 |
| 右摇杆 | 上下/左右滚动 | 浏览网页、文档 |
| Dpad ↑ | 音量增大 | |
| Dpad ↓ | 音量减小 | |
| Dpad ← | 静音/取消静音 | |

### LT层（按住LT扳机）

| 操作 | 效果 | 常用场景 |
|------|------|---------|
| LT+X | 打开Alt+Tab窗口切换器 | 切换应用程序 |
| LT+LB | 在切换器中向左选 | Alt+Shift+Tab |
| LT+RB | 在切换器中向右选 | Alt+Tab前进 |
| LT+A | 回车确认 | 确认选择 |
| LT+B | Escape取消 | 关闭菜单 |
| LT+Y | 关闭当前窗口(Alt+F4) | 关闭程序 |
| LT+L3 | 粘贴(Ctrl+V) | |
| LT+R3 | **显示操作帮助** | 查看所有按键说明 |
| **LT+View(长按1秒)** | **切换鼠标/默认模式** | 切换手柄工作模式 |

### RT层（按住RT扳机）

| 操作 | 效果 | 常用场景 |
|------|------|---------|
| RT+Dpad→ | 下一个标签(Ctrl+Tab) | 浏览器 |
| RT+Dpad← | 上一个标签(Ctrl+Shift+Tab) | 浏览器 |
| RT+Dpad↑ | 向上翻页(PageUp) | 浏览文档 |
| RT+Dpad↓ | 向下翻页(PageDown) | 浏览文档 |
| RT+A | 全选(Ctrl+A) | |
| RT+B | 复制(Ctrl+C) | |
| RT+X | 剪切(Ctrl+X) | |
| RT+Y | 粘贴(Ctrl+V) | |
| RT+RB | 保存(Ctrl+S) | 保存文档 |
| RT+L3 | 撤销(Ctrl+Z) | |
| RT+R3 | 重做(Ctrl+Shift+Z) | |

---

## 窗口切换操作示例

**场景**：在浏览器和记事本之间切换

1. **按住LT扳机不放手**
2. **按一下X** → 屏幕弹出Alt+Tab窗口切换器
3. **按LB向左选**或**按RB向右选** → 在打开的窗口中循环
4. **松开LT或按A** → 确认切换到选中的窗口

---

## 系统托盘操作

右键点击托盘图标，弹出菜单：

| 菜单项 | 功能 |
|--------|------|
| 状态栏 | 显示各手柄模式（如 `P1:M  P2:D`） |
| Switch Mode | 所有手柄切换鼠标/默认模式 |
| Lock Mode | 锁定当前模式（禁止切换） |
| Pause Passthrough | 暂停手柄映射 |
| Exit | 退出程序 |

**双击托盘图标**可快速切换模式。

---

## 配置文件

配置文件位于程序同目录下的 `config.json`。

首次运行自动生成默认配置文件。你可以用记事本打开编辑：

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
    "left_stick": {
      "sensitivity_x": 1.0,
      "sensitivity_y": 1.0,
      "deadzone": 0.15,
      "acceleration": true
    },
    "right_stick": {
      "scroll_speed_vertical": 1.0,
      "scroll_speed_horizontal": 1.0,
      "deadzone": 0.15
    },
    "button_mapping": {
      "A": "MouseLeftClick",
      "B": "MouseRightClick",
      "X": "MouseMiddleClick",
      "Y": "Enter",
      "DpadUp": "VolumeUp",
      "DpadDown": "VolumeDown",
      "DpadLeft": "VolumeMute",
      "DpadRight": "None",
      "LB": "MouseLeftHold",
      "RB": "MouseRightHold",
      "LT": "None",
      "RT": "None",
      "L3": "Enter",
      "R3": "Escape",
      "View": "Tab",
      "Menu": "Win"
    },
    "modifier_mapping": {
      "LT": {
        "X": "Alt+Tab",
        "LB": "ShiftTab",
        "RB": "Tab",
        "A": "Enter",
        "B": "Escape",
        "Y": "Alt+F4",
        "DpadUp": "ArrowUp",
        "DpadDown": "ArrowDown",
        "L3": "Ctrl+V",
        "R3": "ShowHelp"
      },
      "RT": {
        "DpadRight": "Ctrl+Tab",
        "DpadLeft": "Ctrl+Shift+Tab",
        "DpadUp": "PageUp",
        "DpadDown": "PageDown",
        "A": "Ctrl+A",
        "B": "Ctrl+C",
        "X": "Ctrl+X",
        "Y": "Ctrl+V",
        "LB": "None",
        "RB": "Ctrl+S",
        "L3": "Ctrl+Z",
        "R3": "Ctrl+Shift+Z"
      }
    }
  },
  "osd": {
    "enabled": true,
    "duration_seconds": 2
  },
  "autostart": false
}
```

**热加载**：修改配置后保存，程序自动生效，无需重启。

### 常用配置调整

| 配置项 | 作用 | 建议值 |
|--------|------|--------|
| manual_switch_lockout_seconds | 两次切换间隔 | 1-10秒 |
| sensitivity_x/y | 鼠标移动灵敏度 | 0.5-2.0 |
| deadzone | 摇杆死区(越小越灵敏) | 0.10-0.20 |
| acceleration | 是否启用鼠标加速 | true/false |
| hold_duration_ms | 切换模式长按时间 | 500-2000ms |

---

## 常见问题

**Q: 程序无法启动，Windows提示安全问题？**
A: 这是自编译程序的正常现象。点击"更多信息"→"仍要运行"。如果无法通过，可在Windows安全中心关闭"智能应用控制"。

**Q: 手柄没反应？**
A: 确保手柄已连接且支持XInput（Xbox手柄或兼容的第三方手柄）。查看托盘图标状态。

**Q: 如何切换模式？**
A: 长按LT+View 1秒。每个手柄独立切换。

**Q: 在游戏里怎么查看帮助？**
A: 按住LT + 按R3，帮助屏幕在任何模式下都可以显示。

**Q: 多个手柄怎么区分？**
A: 托盘菜单状态栏显示 `P1:M  P2:D`，OSD通知会标明 `Pad1: Mouse`。

**Q: 如何关闭程序自启动？**
A: 编辑config.json，将`autostart`设为`false`。

**Q: 如何查看调试日志？**
A: 运行目录下的`debug.log`文件记录了程序运行日志。
