# 手柄鼠标模拟器 - 实现总结文档

## 项目概述

Windows 平台的后台工具，通过Xbox手柄控制鼠标/键盘操作。按住LT扳机进入Alt组合键模式，按住RT扳机进入Ctrl组合键模式，实现窗口切换、文本编辑、浏览器导航等操作。

---

## 技术栈

| 项目 | 内容 |
|------|------|
| 平台 | Windows 10/11 64位 |
| 语言 | C++17 |
| 框架 | Qt 6.11.1 |
| 编译器 | MinGW 13.1.0 (GCC) |
| 构建系统 | CMake 4.3.3 |
| 手柄API | XInput 1.4 |
| 按键注入 | SendInput / keybd_event |

---

## 项目结构

```
D:\project\sbgj\
├── CMakeLists.txt                  # CMake构建配置
├── .gitignore                      # Git忽略规则
├── readme.md                       # 需求规格说明书
├── config/
│   └── default_config.json         # 默认配置文件(热加载)
├── resources/
│   └── resources.qrc               # Qt资源文件
└── src/
    ├── main.cpp                    # 程序入口+日志系统
    ├── app/
    │   ├── Application.h/cpp       # 顶层协调器(拥有所有子系统)
    ├── core/
    │   ├── Types.h/cpp             # 枚举/结构体/常量
    │   ├── Config.h/cpp            # JSON配置读写+热加载
    │   ├── ModeManager.h/cpp       # 模式状态机(鼠标/默认+锁定/暂停)
    │   └── ProcessDetector.h/cpp   # Win32进程枚举检测
    ├── gamepad/
    │   ├── GamepadPoller.h/cpp     # XInput 60Hz轮询线程
    │   └── ComboKeyDetector.h/cpp  # View+Menu组合键检测
    ├── input/
    │   ├── InputMapper.h/cpp       # 模式分发调度
    │   ├── MouseMapper.h/cpp       # 摇杆→鼠标光标+滚轮
    │   └── KeyboardMapper.h/cpp    # 按钮→键盘映射+修饰层
    ├── ui/
    │   ├── SystemTray.h/cpp        # 系统托盘+右键菜单
    │   └── OsdOverlay.h/cpp        # 半透明OSD通知弹窗
    └── win/
        ├── XInputWrapper.h/cpp     # XInput API封装
        └── SendInputHelper.h/cpp   # SendInput鼠标/键盘辅助
```

---

## 线程模型

```
主线程 (Qt事件循环)
├── Config (QFileSystemWatcher热加载回调)
├── ModeManager (QTimer驱动进程轮询)
├── ProcessDetector (同步调用, ~1ms)
├── ComboKeyDetector (接收GamepadPoller信号)
├── InputMapper → MouseMapper / KeyboardMapper
├── SystemTray / OsdOverlay

GamepadPoller线程 (QThread)
└── XInput轮询循环 (~16ms间隔 = 60Hz)
    通过queued connection发射信号到主线程
```

---

## 核心设计决策

### 1. XInput轮询
- 使用专用QThread以60Hz轮询，主线程通过queued connection接收状态更新
- 原始摇杆值(int16)归一化到[-1,1]，应用圆形死区(24%)后平滑remap

### 2. 修饰键系统
- **LT修饰层**：按住LT时，其他按键走LT修饰映射表
- **RT修饰层**：按住RT时，其他按键走RT修饰映射表
- 修饰键状态通过`m_ltHeld`/`m_rtHeld`跟踪，lookupAction根据状态选择映射

### 3. Alt+Tab窗口切换
- **问题**：XInput Dpad一次物理按下会报告多次press事件，导致Alt+Tab跳多格
- **解决**：
  1. 改用LB/RB替代Dpad做Alt+Tab左右切换（避免Dpad在切换窗口中被识别为方向键）
  2. 第一次触发发送原子Alt+Tab（SendInput：Alt按下+Tab按下+Tab释放，不释放Alt）
  3. 用`m_ltTabBlocked`按钮级去重，Dpad按住时不重复触
  4. 后续触发只发送Tab（Alt已保持）
  5. LT松开时发送Alt释放

### 4. 配置热加载
- QFileSystemWatcher监听config.json变化
- 300ms debounce防抖
- 修改保存后自动生效，无需重启

### 5. 进程检测
- CreateToolhelp32Snapshot枚举进程
- 在主线程QTimer同步调用（~1ms，远小于2秒间隔）
- 检测到目标进程→默认模式(手柄透传)；未检测到→鼠标模式

### 6. 默认模式(透传)
- 不调用SendInput，XInput信号自然被游戏接收
- 仅ComboKeyDetector持续监听View+Menu组合键

---

## 按键映射系统

### 基础层（直接按键）

| 按键 | 映射动作 | 说明 |
|------|---------|------|
| A | MouseLeftClick | 鼠标左键单击 |
| B | MouseRightClick | 鼠标右键单击 |
| X | MouseMiddleClick | 鼠标中键单击 |
| Y | Enter | 回车键 |
| LB | MouseLeftHold | 按住左键(拖拽用) |
| RB | MouseRightHold | 按住右键 |
| L3 | Enter | 回车(左摇杆按下) |
| R3 | Escape | 退出键(右摇杆按下) |
| View | Tab | Tab键 |
| Menu | Win | 开始菜单 |
| Dpad ↑ | VolumeUp | 音量增大 |
| Dpad ↓ | VolumeDown | 音量减小 |
| Dpad ← | VolumeMute | 静音切换 |
| 左摇杆 | 鼠标移动 | 含加速曲线+死区 |
| 右摇杆 | 滚轮 | 垂直/水平滚动 |

### LT修饰层（按住LT时Alt被按下）

| 组合 | 映射动作 | 说明 |
|------|---------|------|
| LT+X | Alt+Tab | 打开窗口切换器 |
| LT+LB | ShiftTab | 切换器向左 |
| LT+RB | Tab | 切换器向右 |
| LT+A | Enter | 确认选择 |
| LT+B | Escape | 取消 |
| LT+Y | Alt+F4 | 关闭窗口 |
| LT+L3 | Ctrl+V | 粘贴 |
| LT+R3 | Ctrl+Z | 撤销 |

### RT修饰层（按住RT时Ctrl被按下）

| 组合 | 映射动作 | 说明 |
|------|---------|------|
| RT+Dpad→ | Ctrl+Tab | 下一标签 |
| RT+Dpad← | Ctrl+Shift+Tab | 上一标签 |
| RT+Dpad↑ | PageUp | 上翻页 |
| RT+Dpad↓ | PageDown | 下翻页 |
| RT+A | Ctrl+A | 全选 |
| RT+B | Ctrl+C | 复制 |
| RT+X | Ctrl+X | 剪切 |
| RT+Y | Ctrl+V | 粘贴 |
| RT+RB | Ctrl+S | 保存 |
| RT+L3 | Ctrl+Z | 撤销 |
| RT+R3 | Ctrl+Shift+Z | 重做 |

---

## 构建与运行

```bash
cd D:\project\sbgj\build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j 8
windeployqt --no-translations GamepadMouseSim.exe
.\GamepadMouseSim.exe
```

---

## 配置文件(default_config.json)

```json
{
  "monitoring": {
    "process_list": ["steam.exe"],
    "polling_interval_seconds": 2,
    "manual_switch_lockout_seconds": 30
  },
  "combo_key": {
    "buttons": ["View", "Menu"],
    "hold_duration_ms": 1000
  },
  "mouse_mode": {
    "left_stick": { "sensitivity_x": 1.0, "sensitivity_y": 1.0,
                     "deadzone": 0.15, "acceleration": true },
    "right_stick": { "scroll_speed_vertical": 1.0,
                      "scroll_speed_horizontal": 1.0, "deadzone": 0.15 },
    "button_mapping": { ... },
    "modifier_mapping": { "LT": { ... }, "RT": { ... } }
  },
  "osd": { "enabled": true, "duration_seconds": 2 },
  "autostart": false
}
```

修改后自动生效，无需重启。