# Proposal: add-virtual-keyboard

## Why

产品的目标场景是"用手柄在沙发/电视上操控桌面 PC"。移动光标、点击、滚动已经完备,唯独**打字**是最大断崖:搜个视频、输个密码、填个 URL 都做不到。召唤系统触摸键盘(TabTip)无法用手柄导航,摇杆点按键体验很差。本 change 落地**手柄可导航的自绘虚拟键盘 overlay**,并预留"内嵌拼音引擎"的演进路径,让打字能力一步到位接入现有架构。

## What Changes

- 新增 `ButtonAction::ShowKeyboard`:切换虚拟键盘 overlay 的显隐,默认绑定 **LT 修饰层 + Menu**(物理手势 = **按住 L3 + 按 Menu**——config 的 "LT" 修饰层由按住左摇杆 L3 进入,见 design.md 层命名说明;与现有 L3+R3=ShowHelp 同构)。
- 新增 **KeyboardOverlay** 子系统:自绘 QWERTY 虚拟键盘窗口;摇杆/十字键在键位间移动高亮,A 确认,B 退格,Menu(或专用键)关闭。
- 键位激活通过 SendInput 把**真实按键**注入到当前持焦窗口;overlay 全程 `WS_EX_NOACTIVATE` **不抢焦点**,文字落入用户原本所在的输入框。
- **IME 透明设计**:键盘布局包含数字行、空格、Esc、退格等标准键——用户若开着微软拼音,组词/选字(数字选候选、空格上屏)由系统输入法照常完成,**本期零 IME 集成代码**(详见 design.md 的 B1/B2 分层)。
- 键盘打开期间,该手柄的输入路由到键盘导航,**不再**驱动鼠标光标;仅 Mouse 模式下可用(Default 模式保持纯透传)。
- 配置变更(附加性):`default_config.json` 的 `mouse_mode.modifier_mapping.LT` 新增 `"Menu": "ShowKeyboard"`;schema 版本不变;对旧配置缺键时的行为见 design.md(合并默认)。
- 修复既有不一致(apply 阶段发现):`SettingsDialog` 的 L3 层映射此前读写 `mouse_mode.modifier_mapping.L3`,而运行时只读 `LT` 层——GUI 改的 L3 层映射从不生效(死配置);已统一为 `LT` 键路径,使"绑定可通过设置 GUI 自定义"成立。
- 文档链:`README_CN.md` 操作表 + `resources/icons/help.png` 重新生成 + 架构 spec/CLAUDE.md 图更新 + config 迁移说明。
- **明确不在本期**:B2 内嵌拼音引擎(librime/libgooglepinyin)、候选栏自绘、召唤 TabTip(A 路线)。这些作为设计中的演进路径记录,不实现。

## Capabilities

### New Capabilities

- `virtual-keyboard`: 手柄可导航的虚拟键盘 overlay。覆盖:开关触发与模式约束、摇杆/十字键导航与确认、按键注入(焦点纪律)、IME 透明性(标准键覆盖组词选字)、打开期间的手柄输入路由、多手柄行为、 dismiss(关闭)途径。

### Modified Capabilities

- (无 —— `openspec/specs/` 目前为空,不存在需修改的既有规格。)

## Impact

- **新增**:`src/ui/KeyboardOverlay.{h,cpp}`(overlay 窗口 + 键位导航)、`tests/` 下对应 Qt Test(模拟优先,Fake 注入器)。
- **修改**:
  - `src/core/Types.{h,cpp}`:`ButtonAction::ShowKeyboard` + `stringToAction`/`actionToString`/`actionToChinese`。
  - `src/app/Application.cpp`:KeyboardOverlay 与 GamepadPoller/InputMapper/ModeManager 的接线(唯一的 wiring 文件)。
  - `src/input/InputMapper` / `KeyboardMapper`:键盘打开期间的路由旁路 + 新动作分发。
  - `src/win/SendInputHelper`:如现有 API 不覆盖"注入字母键/特殊键到前台"则扩展。
  - `src/ui/SettingsDialog.cpp`:`ShowKeyboard` 加入 `kActionOrder`(动作下拉自动获得);L3 层键路径修复(`L3` → `LT`,见 What Changes)。
  - `config/default_config.json`:LT 层 Menu 默认映射。
  - `tools/generate_help_png.py` 输入表 + `resources/icons/help.png`(重新生成)。
  - `README_CN.md`、`docs/superpowers/specs/2026-07-13-code-architecture-design.md`、`CLAUDE.md` 架构图、`docs/superpowers/specs/config-release-design.md` 迁移说明。
- **风险与限制**:UIPI——提权窗口收不到非提权进程的注入(与产品现有模拟输入限制一致);注入按"当前前台窗口"进行,前台切换瞬间存在竞态(见 design.md 焦点纪律一节)。
