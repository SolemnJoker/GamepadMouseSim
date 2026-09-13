# Design: add-virtual-keyboard

## Context

- 产品形态:托盘常驻、无主窗口;已有全屏无边框窗口先例 `OsdOverlay`(OSD 通知),证明"叠加窗口"路线在本项目可行。
- 输入链路:`GamepadPoller`(私有线程 60Hz)→ `InputMapper`/`MouseMapper`/`KeyboardMapper` → `SendInputHelper` → 系统。全部 wiring 在 `Application.cpp`。
- `ButtonAction` 枚举是按钮→动作映射的规范源;设置 GUI 的下拉框遍历 `kActionOrder` 自动生成,**新增枚举值零 GUI 成本**。
- 本期动机与范围见 proposal.md;行为契约见 specs/virtual-keyboard/spec.md。

## Goals / Non-Goals

**Goals:**

- B1 期:手柄可导航的自绘虚拟键盘,键位以"物理键盘等价"方式注入前台窗口;系统输入法(微软拼音等)在目标窗口照常组词选字,零 IME 集成代码。
- 为 B2(内嵌拼音引擎、自绘候选栏)预留干净分层:换引擎层,外壳与递交接口不动。
- 满足 AGENTS.md Rule 1(文档链)与 Rule 2(模拟优先测试)。

**Non-Goals:**

- 不实现 B2 引擎(librime/libgooglepinyin 链接、候选栏 UI)。
- 不实现"召唤系统触摸键盘 TabTip"(A 路线)。
- 不做系统级输入法注册/TSF 集成。
- 不支持鼠标直接点击键盘按键(v1 可选增益,见 Open Questions)。

## Decisions

### D1 — 自绘 overlay,不召唤 TabTip

手柄原生导航是本能力的核心诉求(用户旅程断崖的解法)。TabTip 是黑盒窗口,无法接管其键位导航。自绘 overlay 同时是 B1/B2 两期共用的外壳,一次投入两期受益。

**备选**:A 路线(ITipInvocation 唤起 TabTip)——成本低但导航体验差、Win11 22H2+ 唤起方式不稳定,降级为"逃生门"价值,本期不做。

### D2 — B1 核心洞察:IME 透明,盲组词

虚拟键盘只做一件事:**把键位变成与物理键盘等价的真实按键事件**。目标窗口的输入法(用户已配置的)按物理键盘规则处理这些事件——字母组词、数字选候选、空格上屏、Esc 取消。我们**不读取、不渲染、不感知**候选词("盲组词"),因为候选框由系统画在目标窗口旁,用户看得见。

**为什么这样够了**:打字闭环所需的一切键位(字母/数字/空格/退格/Esc/回车)都在键盘布局上,候选操作 = 导航到数字键按 A,无需任何特殊"候选模式"。

**备选**:
- *TSF headless 驱动系统输入法*(在自身进程喂键取候选):COM 深坑、文档稀少,拒绝。
- *内嵌引擎*(即 B2):体验最优但依赖重,作为演进期而非起步,见 D3。

### D3 — 两期分层:B1→B2 演进路径

```
             ┌────────────────────────────────────────────┐
             │ KeyboardOverlay (Qt 窗口:绘制+高亮)          │
             │ KeyboardNavController(纯逻辑:摇杆→格子移动) │
             │        B1 与 B2 完全共用 ↑                   │
             ├────────────────────────────────────────────┤
   B1:  字母/数字/功能键 → KeyInjector.sendVk()   ────→ 前台窗口 → 系统 IME 组词(系统画候选框)
   B2:  字母/数字 → PinyinEngine(本进程) → 候选栏画进 overlay → KeyInjector.commitText() ────→ 前台窗口
             │        B2 仅替换"引擎层",并给递交层加一个方法 ↓ │
             └────────────────────────────────────────────┘
```

- **KeyInjector** 是递交层接口:`sendVk(vk, modifiers)`(B1 用)+ 预留 `commitText(QString)`(B2 用,走 `KEYEVENTF_UNICODE` 通道,组词后直接落字)。
- B2 落地时:overlay 增加"候选栏"组件(导航逻辑已有,PinyinEngine 吐候选字符串),KeyInjector 增加 commitText,外壳零改动。**本 change 只要求把 KeyInjector 抽成可注入接口,FakeKeyInjector 用于测试,B2 实现类后续另立 change。**

### D4 — 注入通道必须走 VK+scancode,不能用 KEYEVENTF_UNICODE

`KEYEVENTF_UNICODE`(VK_PACKET)注入的是"已定字符",会**绕过**输入法组词——这正是它"能直接打中文"的原理,但也意味着系统 IME 永远没机会组词。B1 必须发**带 VK 码 + scancode 的真实按键序列**,IME 才会拦截并组词。UNICODE 通道是 B2 的 `commitText` 用的(引擎自己组词、跳过系统 IME)。

两通道归属不同期、同一接口不同方法——这是分层正确性的关键,写在这里防止实现时用错。

### D5 — 焦点纪律与窗口姿势

- overlay 窗口:`WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW` + `Qt::WindowStaysOnTopHint`,show/hide 全程不调 `activateWindow`/`setFocus`。与 TabTip 同姿势。
- 键盘打开时**记录当时的 `GetForegroundWindow()`**(仅用于日志诊断);注入保持"物理键盘语义"——发往**注入那一刻**的前台窗口(用户中途 Alt-Tab 换了前台,字落新前台,与物理键盘一致,不做事后校验)。
- 视图层分离(实现细化,2026-09-12):`KeyboardOverlay` 实现 `src/input/KeyboardController.h` 中的 `IKeyboardOverlay` 接口、只读 `KeyboardNavController` 的布局/高亮用于绘制——ui→input 的**只读视图**依赖作为例外记入架构 spec §2.2(2026-09-12 增补),行为全在 input 层;单测经 `FakeOverlay` 驱动控制器,不创建真实窗口。

### D6 — 键盘打开期间的手柄输入路由

- 旁路位置:`Application` 的 `gamepadStateChanged` 分发处。overlay 可见时,原始 `GamepadState` 交给 `KeyboardController`(摇杆/十字键量化导航 + A 确认/B 退格),**不再**进入 `InputMapper`。
- 两个全局出口必须存活:模式切换组合(L3+View,独立的 ComboKeyDetector,在分发序列中先于旁路执行,不受影响)与键盘关闭途径(绑定 toggle + 键盘上的 ✕ 键)。其余输入被键盘吞掉,防止"开键盘时误点鼠标"。
- toggle 检测拆两半(实现细化,2026-09-12):**打开** = InputMapper 在 Mouse 模式下识别 ShowKeyboard 动作 → `showKeyboardRequested` 信号 → `openOverlay()`——overlay 关闭时 InputMapper 未被旁路,直接复用其既有层状态机,避免在旁路层重复实现一套修饰键跟踪;**关闭** = overlay 打开期间 InputMapper 已被旁路,由 `KeyboardController` 反查配置中 ShowKeyboard 的当前绑定(修饰层 + 按钮上升沿)来关闭。打开/关闭各只有一个检测点,无双重触发。
- 摇杆 Y 轴取向:XInput 原始 Y 轴**向上为正**,屏幕坐标向下为正——键盘导航与 MouseMapper 一致对 Y 取反(上推 = 高亮上移)。回归测试 `stick_upMovesHighlightUp` 固定该行为(2026-09-12 真机反馈修复)。

### D7 — 布局数据驱动 + Shift 语义

- 布局 = 数据表(QVector<行>,每键 {显示标签, VK, 宽度}):QWERTY 四行 + 数字行 + 底行(Shift/空格/退格/Esc/回车/关闭)。布局与窗口尺寸解耦,改布局不改代码逻辑。
- **Shift 单击的 IME 歧义坑**:物理键盘"单击 Shift"会被微软拼音/搜狗解释为"中英文切换"而非大写。规避:键盘上的 Shift 键实现为 **sticky 修饰**——点亮后,下一个字母键以 Shift+字母 **组合**注入(组合不会触发 IME 的中英切换,与物理键盘按住 Shift 打字行为一致),用完自动熄灭。绝不单独发送 Shift 单击。

### D8 — 配置默认值合并

`default_config.json` 的 LT 层新增 `"Menu": "ShowKeyboard"`。**旧用户配置缺该键时的行为:回退到代码内置默认表**(与 default_config 同源)。这保证升级用户零操作获得新绑定。落地机制(2026-09-12 实现):`KeyboardMapper::loadConfig` 以代码内置默认表(直接映射 / LT 层 / RT 层各一张)起步,再按键覆盖配置里**存在**的键——缺失键保留默认,显式 `"None"` 按用户意图生效,未知层名以空表起步。schema 版本不变(纯附加)。

层命名说明(实现时确认):config 的 `LT` 修饰层物理上由**按住 L3(左摇杆按下)**进入,`KeyboardMapper::lookupAction` 以 `m_l3Held` 判定——"LT 层 + Menu"的物理手势即**按住 L3 + 按 Menu**。`KeyboardController` 的绑定反查同时兼容 `LT` 与历史 `L3` 层名。

### D9 — 测试策略(Rule 2 模拟优先)

- `KeyInjector` 接口化,`FakeKeyInjector` 记录调用序列 → 覆盖:字母/数字/功能键注入、Shift 组合序列、B2 通道占位。
- `KeyboardNavController` 为纯逻辑类(不碰窗口),用合成的 GamepadState 序列驱动 → 覆盖:四方向移动边界、重复延迟、A 确认、B 退格。
- **逻辑与窗口严格分离**:单元测试不创建真实 QWidget(仓库已知坑:offscreen QPA 下 tray-icon 测试栈溢出被 ctest 排除——同类风险规避)。
- Manual smoke(需 spec 批准 carve-out + follow-up issue):真实前台焦点保持、真实微软拼音组词视觉、多显示器显示位置。

### D10 — 文档链(Rule 1)

新 `ButtonAction` → `README_CN.md` 操作表 + `tools/generate_help_png.py` 重新生成 `help.png`;新子系统 → `docs/superpowers/specs/2026-07-13-code-architecture-design.md` + `CLAUDE.md` 架构图;配置默认值 → `default_config.json` + `config-release-design.md` 迁移说明。

## Risks / Trade-offs

- [UIPI:提权窗口收不到非提权进程注入] → 与产品现有模拟输入限制一致,README 已有说明;不新增检测。
- [第三方输入法对标准键绑定差异(如搜狗)] → B1 只依赖"数字选候选/空格上屏/Esc 取消"等各家通行的标准绑定;若某 IME 不兼容,英文/密码场景仍完全可用。
- [前台竞态:注入瞬间前台切换] → 接受"物理键盘语义"(跟随当前前台);日志记录打开时 hwnd 便于诊断。
- [摇杆导航手感(步进速度/重复延迟)需调校] → 参数进 config(`keyboard` 节),默认值保守,后续可调。
- [offscreen QPA 测试栈溢出(仓库已知)] → D9 逻辑/窗口分离,单测不建真实窗口。

## Migration Plan

纯附加变更:新枚举值、新配置默认键(缺失回退代码默认)、新子系统文件。回滚 = 移除 LT.Menu 绑定 + 隐藏入口即可,无数据迁移。

## Open Questions

- 键盘是否同时接受真实鼠标点击(不激活窗口的前提下处理 click)——v1 可选增益,不阻塞,实施时按余量决定。
- 摇杆导航的"斜方向"是否允许对角移动(布局网格化后自然支持),默认实现四方向 + 对角,调优阶段裁剪。
