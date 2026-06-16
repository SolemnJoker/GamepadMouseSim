# 手柄鼠标模拟器 - 实现总结文档

## 项目概述

Windows 平台后台工具，将 Xbox 手柄映射为鼠标/键盘操作。支持最多 4 个手柄同时使用，每个手柄独立管理模式。通过 LT/RT 扳机进入 Alt/Ctrl 组合键模式。

---

## 技术栈

| 项目 | 内容 |
|------|------|
| 平台 | Windows 10/11 64位 |
| 语言 | C++17 |
| 框架 | Qt 6.11.1 |
| 编译器 | MinGW 13.1.0 (GCC) |
| 构建系统 | CMake |
| 手柄API | XInput 1.4 |
| 按键注入 | SendInput / keybd_event |
| 安装包 | Inno Setup 6 |

---

## 项目结构

```
D:\project\sbgj\
├── CMakeLists.txt                  # CMake构建配置
├── .gitignore                      # Git忽略规则
├── readme.md                       # 需求规格说明书
├── config/
│   └── default_config.json         # 默认配置文件(热加载)
├── installer/
│   └── GamepadMouseSim.iss         # Inno Setup安装包脚本
├── docs/
│   └── compose/                    # 设计文档
├── resources/
│   └── resources.qrc               # Qt资源文件
└── src/
    ├── main.cpp                    # 程序入口+日志系统
    ├── app/
    │   └── Application.h/cpp       # 顶层协调器(4组子系统实例)
    ├── core/
    │   ├── Types.h/cpp             # 枚举/结构体/常量(含kMaxGamepads=4)
    │   ├── Config.h/cpp            # JSON配置读写+热加载
    │   └── ModeManager.h/cpp       # 模式状态机(每手柄独立实例)
    ├── gamepad/
    │   ├── GamepadPoller.h/cpp     # XInput 60Hz轮询线程(0-3号控制器)
    │   └── ComboKeyDetector.h/cpp  # LT+View长按组合键检测(每手柄独立)
    ├── input/
    │   ├── InputMapper.h/cpp       # 模式分发+Help信号路由
    │   ├── MouseMapper.h/cpp       # 摇杆→鼠标光标+滚轮
    │   └── KeyboardMapper.h/cpp    # 按钮→键盘映射+修饰层+帮助文本生成
    ├── ui/
    │   ├── SystemTray.h/cpp        # 系统托盘+多手柄状态显示
    │   └── OsdOverlay.h/cpp        # OSD通知+全屏帮助覆盖层(85%)
    └── win/
        ├── XInputWrapper.h/cpp     # XInput API封装
        └── SendInputHelper.h/cpp   # SendInput鼠标/键盘辅助
```

---

## 架构设计

### 多手柄架构

```
Application
├── Config ×1 (全局共享)
├── GamepadPoller ×1 (轮询0-3号控制器)
├── SystemTray ×1 (显示所有手柄状态)
├── OsdOverlay ×1 (模式通知+帮助屏幕)
│
├── Pad 0: ComboKeyDetector[0] → ModeManager[0] → InputMapper[0]
├── Pad 1: ComboKeyDetector[1] → ModeManager[1] → InputMapper[1]
├── Pad 2: ComboKeyDetector[2] → ModeManager[2] → InputMapper[2]
└── Pad 3: ComboKeyDetector[3] → ModeManager[3] → InputMapper[3]

每个InputMapper包含: MouseMapper ×1 + KeyboardMapper ×1
```

每个手柄拥有一套完整的状态：
- 独立的 `ComboKeyDetector`：检测 LT+View 长按 → 切换模式
- 独立的 `ModeManager`：维护当前模式(鼠标/默认) + 锁定期
- 独立的 `InputMapper`：维护 LT/RT 修饰键状态、按键前值

### 线程模型

```
主线程 (Qt事件循环)
├── Config (QFileSystemWatcher热加载回调)
├── ComboKeyDetector[0..3]
├── InputMapper[0..3] → MouseMapper / KeyboardMapper
├── ModeManager[0..3] (仅锁定期QTimer)
├── SystemTray / OsdOverlay

GamepadPoller线程 (QThread)
└── XInput轮询循环 (~16ms间隔 = 60Hz)
     for (i = 0..3): XInputGetState(i) → emit gamepadStateChanged(i, state)
     通过queued connection发射信号到主线程
```

---

## 核心设计决策

### 1. 多手柄独立状态

- `GamepadPoller` 循环轮询 0-3 号控制器，信号携带 `controllerIndex`
- `Application` 创建 4 组子系统实例数组，lambda 按 index 路由信号
- 每个手柄独立维护 modifier 状态、模式、按键前值
- `ModeManager::modeChanged` 信号带 `controllerIndex`，OSD 和托盘按手柄编号显示

### 2. 手动模式切换（无自动检测）

- **已移除**：进程检测自动切换模式（原先通过 CreateToolhelp32Snapshot）
- 启动后默认为鼠标模式，切换完全手动
- 组合键：**长按 LT+View 1秒**
- `ComboKeyDetector` 检测 LT 扳机(>0.5阈值) + View 按钮
- 锁定期 3 秒防止连续误触

### 3. LT/RT 修饰层系统

- **processTrigger** 在**所有模式下都运行**，保证修饰键状态跟踪不丢失
- **processButton** 仅在鼠标模式下运行（默认模式仅处理R3用于帮助）
- 模式切换时调用 `releaseModifiers()` 强制释放 Alt/Ctrl，防止键盘卡键
- LT+View 组合键被 KeyboardMapper 拦截（不触发 Alt+Tab）

### 4. Alt+Tab 窗口切换

- 第一次触发发送原子 Alt+Tab（Alt按下+Tab按下+Tab释放，Alt保持）
- 后续触发只发送 Tab（keybd_event），Alt 保持按下
- 按钮级去重 `m_ltTabBlocked` 防止重复触发
- LT 松开时发送 Alt 释放

### 5. 帮助屏幕

- 触发：LT+R3（任何模式下可用）
- 显示：85% 全屏居中覆盖层，中文，25pt 微软雅黑
- 布局：多列自动计算（按窗口高度和行数计算列数）
- 章节标题高亮：橙色(##)、蓝色(#)
- 5 秒后自动淡出

### 6. 配置热加载

- QFileSystemWatcher 监听 config.json 变化
- 300ms debounce 防抖
- 修改保存后所有 4 组 InputMapper/ModeManager 同步重载

### 7. 默认模式(透传)

- 不调用 SendInput（鼠标和按键都不处理）
- 仅保留 processTrigger（跟踪LT/RT状态）+ R3按钮（用于帮助屏幕）
- ComboKeyDetector 持续监听 LT+View 组合键（不受模式影响）

---

## 按键映射

### 直接映射

| 按键 | 动作 | 说明 |
|------|------|------|
| A | MouseLeftClick | 鼠标左键单击 |
| B | MouseRightClick | 鼠标右键单击 |
| X | MouseMiddleClick | 鼠标中键单击 |
| Y | Enter | 回车键 |
| LB | MouseLeftHold | 按住左键 |
| RB | MouseRightHold | 按住右键 |
| L3 | Enter | 回车 |
| R3 | Escape | 退出 |
| View | Tab | Tab键 |
| Menu | Win | 开始菜单 |
| Dpad↑ | VolumeUp | 音量增大 |
| Dpad↓ | VolumeDown | 音量减小 |
| Dpad← | VolumeMute | 静音 |

### LT层（按住LT）

| 组合 | 动作 | 说明 |
|------|------|------|
| LT+X | Alt+Tab | 窗口切换器 |
| LT+LB | ShiftTab | 切换器向左 |
| LT+RB | Tab | 切换器向右 |
| LT+A | Enter | 确认 |
| LT+B | Escape | 取消 |
| LT+Y | Alt+F4 | 关闭窗口 |
| LT+L3 | Ctrl+V | 粘贴 |
| LT+R3 | **ShowHelp** | 显示帮助 |
| LT+View | 切换模式 | 长按1秒 |

### RT层（按住RT）

| 组合 | 动作 | 说明 |
|------|------|------|
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

### 制作安装包

```bash
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\GamepadMouseSim.iss
```

输出：`installer\GamepadMouseSim-setup.exe`

---

## 初始化流程

```
Application::initialize()
├── Config::load() → 加载 config.json
├── InputMapper[0..3]::onConfigChanged() → 加载按键映射
├── 连接信号槽:
│   ├── GamepadPoller → ComboKeyDetector[0..3] + InputMapper[0..3]
│   ├── ComboKeyDetector → ModeManager (模式切换)
│   ├── ModeManager → InputMapper + SystemTray + OsdOverlay
│   ├── InputMapper → OsdOverlay (帮助屏幕)
│   ├── SystemTray → ModeManager (菜单操作)
│   └── Config → 全部子系统 (热加载)
├── GamepadPoller::start() → 启动轮询线程
├── ModeManager[0..3]::start() → 初始化模式
└── SystemTray::show() → 显示托盘图标
```
