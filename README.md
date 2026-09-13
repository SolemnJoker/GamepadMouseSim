# GamepadMouseSim · 手柄鼠标模拟器

**坐在沙发上,用 Xbox 手柄操控你的 Windows 桌面。**
*A couch-ready Windows tray app that turns an Xbox (XInput) controller into a mouse, keyboard and media remote — with a gamepad-navigable on-screen keyboard, IME-transparent Chinese input, and switchable operation profiles.*

🎮 用手柄移动光标、点击、滚动、打字、放音乐;进游戏一键交还手柄原始信号。

---

## ✨ 功能

- **鼠标 / 键盘模拟** — 左摇杆移动光标(含加速曲线),右摇杆滚动;16 个物理按键 + L3 / RT 两层修饰,全部可自定义映射
- **虚拟键盘(手柄可导航)** — `L3+Menu` 召唤全屏键盘:摇杆/十字键移动高亮、A 确认、B 退格;开启输入法(微软拼音等)后按物理键盘规则组词、数字键选字 —— **零输入法集成,天然兼容中文**
- **多套操作方案(Profile)** — 为"桌面 / 媒体播放 / 浏览器"保存不同按键方案;托盘菜单一键切换;设置界面可新建 / 重命名 / 删除 / 恢复默认
- **帮助屏实时渲染** — `L3+R3` 呼出按键速查,内容按当前生效映射动态生成,改配置立即反映
- **双模式** — 鼠标模式 ⇄ 透传模式;组合键手动切换(默认 `L3+View` 长按 1 秒);可选自动切换(游戏进程 / 全屏 / CPU / GPU 检测,默认关闭)
- **配置健壮** — JSON 热加载(改完即生效);裸机部署自动生成配置;配置损坏自动退化为出厂默认;托盘一键恢复默认配置(带确认)
- **开机自启动** — 设置界面勾选即可(用户级注册表,免管理员);挪动/重命名程序目录后自启自动自愈;安装包提供可选的自启动勾选,卸载自动清理
- **多手柄** — 最多 4 支手柄,每支独立模式
- **进程级自检** — `GamepadMouseSim.exe --selftest` 对配置链做全套自检(退出码 0 = 通过),可直接接入 CI

## 🚀 快速开始

### 环境要求

- Windows 10/11 64 位
- XInput 兼容手柄(Xbox 手柄或第三方兼容款)

### 从源码构建

```bash
# 依赖:Qt 6(Widgets + Svg,项目基于 6.8.3 开发)、MSVC 2022、CMake ≥ 3.20
# -DCMAKE_PREFIX_PATH 指向你的 Qt 安装目录(含 bin/、lib/cmake/Qt6 的那一层)
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure   # 自动化测试全绿
```

Qt 路径也可以通过 `CMAKE_PREFIX_PATH` 环境变量提供;仓库内没有任何硬编码的本机路径。构建后直接运行 `build/Release/GamepadMouseSim.exe`,配置文件会自动生成。

**分发**:`powershell skills/deploy/deploy.ps1 -TargetDir <目录>` 产出自足的部署目录(Qt DLL、Qt 插件、VC++ 运行库 DLL 全部随包,裸 Windows 10/11 开箱即用,无需安装任何运行库),同时把 `dist/` 暂存为安装器打包源(`ISCC installer\GamepadMouseSim.iss` 产安装包,内含可选的开机自启动勾选)。两种分发方式都会自动排除用户数据(`config.json`/`debug.log`)。程序未做代码签名,目标机首次运行若触发 SmartScreen,点"更多信息 → 仍要运行"。

## 🎮 基本操作

| 按键 | 功能 |
|---|------|
| A | 鼠标左键 |
| B | 鼠标右键 |
| X | 鼠标中键 |
| LB | 按住左键（配合摇杆拖拽） |
| RB | 按住右键（配合摇杆拖拽） |
| Y | 回车 |
| R3 | 退出 |
| View | Tab |
| Menu | 开始菜单 |

> L3 = 左摇杆按下。完整按键表见程序内帮助屏（L3+R3）——它永远与当前生效映射一致。

### L3 层（按住 L3）

| 组合 | 功能 |
|---|------|
| L3+X | Alt+Tab |
| L3+LB | Shift+Tab |
| L3+RB | Tab |
| L3+A | 回车 |
| L3+B | 退出 |
| L3+Y | 关闭窗口 |
| L3+DpadUp | 音量+ |
| L3+DpadDown | 音量- |
| L3+DpadLeft | 静音 |
| L3+R3 | 显示帮助 |
| L3+Menu | 虚拟键盘 |

### RT 层（按住 RT）

| 组合 | 功能 |
|---|------|
| RT+A | 全选 |
| RT+B | 复制 |
| RT+X | 剪切 |
| RT+Y | 粘贴 |
| RT+DpadUp | 上翻页 |
| RT+DpadDown | 下翻页 |
| RT+DpadLeft | 关闭标签（反向） |
| RT+RB | 保存 |
| RT+L3 | 显示桌面 |
| RT+R3 | 撤销 |

<sup>表格由 scripts/sync_docs.py 从按键映射规范自动生成，请勿手工编辑。</sup>

## ⚙️ 配置

首次运行在 exe 旁生成 `config.json`,修改后自动热生效。要点:

- **界面语言**:设置→常规→"界面语言"(跟随系统 / 中文 / English;默认跟随系统)。托盘、OSD、帮助屏在切换后即时生效;设置对话框重开后完全生效
- **操作方案**:托盘菜单 → "操作方案"子菜单切换;设置界面可新建(复制当前)/ 重命名 / 删除
- **托盘"恢复默认配置"**:丢弃全部自定义(含所有方案),恢复出厂(带确认)
- **配置自愈**:文件损坏自动退化为出厂默认并重写;旧版本配置升级时用户自定义**全部保留**(增量合并)
- `--selftest`:排查部署问题时先跑它,每项输出 `[PASS]/[FAIL]`

## 📁 项目文档

| 文档 | 内容 |
|------|------|
| [USER_MANUAL.md](USER_MANUAL.md) | 使用说明书(详细操作与 FAQ) |
| [docs/requirements-spec.md](docs/requirements-spec.md) | 原始需求规格(历史文档) |
| [docs/superpowers/specs/](docs/superpowers/specs/) | 架构与各领域设计规格 |
| [openspec/specs/](openspec/specs/) | 能级行为契约(virtual-keyboard、config-profiles、live-help-screen) |
| [docs/followups/](docs/followups/) | manual-smoke 清单与模拟化方案 |
| [AGENTS.md](AGENTS.md) / [CLAUDE.md](CLAUDE.md) | 开发者约定与架构速览 |

## ⚠️ 已知限制

- 模拟输入无法送达**管理员权限窗口**(Windows UIPI 限制);需要时以管理员身份运行本程序
- 仅支持 XInput 协议手柄(DirectInput / DualSense 原生协议不在支持范围)
- 帮助屏与虚拟键盘为中文界面

## 📄 License

MIT
