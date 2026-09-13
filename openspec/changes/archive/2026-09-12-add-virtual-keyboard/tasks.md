# Tasks: add-virtual-keyboard

> 设计依据:design.md(D1–D10);行为契约:specs/virtual-keyboard/spec.md。
> 测试基线:逻辑/窗口分离(D9),单元测试不创建真实 QWidget。

## 1. 核心类型与接口

- [x] 1.1 `src/core/Types.{h,cpp}`:新增 `ButtonAction::ShowKeyboard` 与 `stringToAction`/`actionToString`/`actionToChinese`(中文文案"虚拟键盘")三处登记,并加入 `kActionOrder`;验证:`tests/` 中现有 Types 相关测试跑通 + 新增一条 string↔action 往返断言
- [x] 1.2 新建 `src/input/KeyInjector.{h,cpp}`:`sendVk(vk, modifiers)` 接口(纯虚)+ `FakeKeyInjector`(记录调用序列,tests/ 下);验证:Fake 的记录/查询单测通过,`ctest` 全绿
- [x] 1.3 在 `KeyInjector.h` 中为 B2 预留 `commitText(QString)` 纯虚声明与注释(design D3/D4),不实现;验证:编译通过,注释写明"UNICODE 通道,绕过 IME,B2 使用"

## 2. 键盘导航纯逻辑

- [x] 2.1 新建 `src/input/KeyboardNavController.{h,cpp}`:布局数据表(QWERTY+数字行+底行特殊键,design D7)、高亮坐标状态、摇杆/十字键量化为四方向(+对角)步进、重复延迟参数;验证:新增 `tests/test_keyboard_nav.cpp`(Qt Test),覆盖四方向移动、行尾边界、重复延迟触发,不创建 QWidget
- [x] 2.2 `KeyboardNavController` 确认/退格/关闭语义:A=确认当前键位(产出该键 {vk, needsShift}),B=退格;验证:单测断言 FakeKeyInjector 收到的 VK 序列正确(字母、数字、空格、Esc、回车、退格各一条)
- [x] 2.3 sticky Shift 逻辑(design D7):点亮 Shift 后下一个字母以 Shift+字母组合注入并自动熄灭,绝不单独发送 Shift;验证:单测断言组合序列且无孤立 Shift 事件

## 3. Overlay 窗口

- [x] 3.1 新建 `src/ui/KeyboardOverlay.{h,cpp}`:无边框置顶窗口,`WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW`(design D5),按布局数据表绘制键位与高亮;打开时记录 `GetForegroundWindow()` 仅入日志;验证:编译通过;manual smoke 项(真实焦点保持)登记到 7.2
- [x] 3.2 overlay 显示尺寸/DPI 适配(主屏居中,固定缩放 v1);验证:1080p 下截图人工确认键位不重叠(manual,登记 7.2);逻辑部分由 2.x 单测覆盖

## 4. 输入路由与接线

- [x] 4.1 `src/app/Application.cpp`:新增 overlay 可见时的输入旁路——原始 `GamepadState` 交 `KeyboardNavController`,不再进入 `InputMapper`;模式切换组合(LT+View combo detector)与 toggle 检测保持存活(design D6);验证:新增集成单测(合成 GamepadState 序列驱动 wiring 层,断言旁路时 MouseMapper 无输出、导航有响应)
- [x] 4.2 toggle 检测:旁路层内实现 LT+Menu(可配置动作)开合检测,关闭后恢复常规路由;Default 模式忽略 toggle、切换到 Default 时自动关闭 overlay;验证:单测覆盖"打开→关闭→路由恢复"、"Default 模式触发无效"、"模式切换自动关闭"三个场景(spec 对应)
- [x] 4.3 多手柄:任一已连接手柄的导航输入作用于同一 overlay 实例;验证:单测以两个 pad 索引的状态序列断言同一 NavController 收到输入

## 5. 配置

- [x] 5.1 `config/default_config.json`:LT 层新增 `"Menu": "ShowKeyboard"`,新增 `keyboard` 节(导航重复延迟等参数,默认保守值);确认 `InputMapper` 对缺失映射键回退到代码内置默认表(旧配置零操作获得新绑定,design D8),不足则补 fallback;验证:新增配置合并单测(无 keyboard 节/无 LT.Menu 的旧配置 → 行为仍正确);热加载冒烟
- [x] 5.2 `docs/superpowers/specs/config-release-design.md` 迁移说明追加条目(schema 不升级,纯附加 + fallback 规则);验证:文档 diff 自查

## 6. 文档链(Rule 1)

- [x] 6.1 `tools/generate_help_png.py` 输入表加 LT+Menu=虚拟键盘,重新生成 `resources/icons/help.png`;验证:生成脚本运行成功、图中含新条目
- [x] 6.2 `README_CN.md`:特性列表 + 操作表新增条目与已知限制(UIPI/提权窗口);验证:文档自查
- [x] 6.3 `docs/superpowers/specs/2026-07-13-code-architecture-design.md` 子系统清单与 `CLAUDE.md` 架构图加入 KeyboardOverlay/KeyInjector/KeyboardNavController;验证:文档 diff 自查

## 7. 验证与收尾

- [x] 7.1 `cmake --build` + `ctest` 全量通过(含本 change 新增测试);验证:本地全绿输出
- [x] 7.2 manual smoke 清单执行并按 Rule 2 登记 carve-out:真实前台焦点保持、真实微软拼音组词选字、Esc 取消组词、1080p 显示;创建 follow-up issue 跟踪"smoke 步骤模拟化";验证:issue 链接写回本文件 → gh CLI 不可用,登记于 docs/followups/virtual-keyboard-manual-smoke.md(含每步模拟化方案),待用户转录为 GitHub issue
- [x] 7.3 走查 `PULL_REQUEST_TEMPLATE.md` 合并清单,确认 Rule 1 文档链(1.1/5.1/6.x)与 Rule 2 测试证据齐备;验证:清单逐行勾选
