# Proposal: add-ui-i18n

## Why

程序的所有界面文案(设置、托盘、帮助屏、OSD)目前是硬编码中文,非中文用户无法使用。README 门面化后仓库已面向国际 GitHub 用户,产品需要配套的多语言 UI 能力。本期落地 **i18n 架构 + 英文语言包**,后续新增语言(如日语)只加一张表。

现状盘点(2026-09-13 实测):中文文案共约 **142 条**,分布——`SettingsDialog` 74 条、`Types.cpp actionToChinese` 38 条(按键动作文案)、`HelpContent` 12 条、`SystemTray` 9 条、`OsdOverlay`/`KeyboardController`/`Application`(确认框)各 3 条;`KeyboardOverlay` 的键帽是字母/符号无需翻译;安装器已随 `ChineseSimplified.isl`/`Default.isl` 双语(不在本期范围)。

## What Changes

- 新增 `src/core/Translations.{h,cpp}`(编入 `src_core`):轻量运行时翻译表,**中文原文为 key**,`tr(zh)` 按当前语言返回译文;缺 key 时 fail-safe 回退中文原文(界面永远不出现裸 key/空串)。
- **语言选择**:`ui.language` 配置键(`"system"`(默认,按系统 locale 自动选 zh/en)/`"zh"`/`"en"`);设置界面"常规"页新增语言下拉,写入即热生效。
- **热切换范围**(关键行为,spec 明确):托盘菜单、OSD、帮助屏、虚拟键盘、确认对话框——**显示时查表,即时生效**;按键动作文案(`actionToChinese` 内部经翻译表)同步双语,帮助屏与设置下拉自动跟随;**设置对话框自身**的静态文案在对话框重建(重新打开)后生效——弹出的对话框在语言切换后提示"重新打开设置以更新语言"。
- `actionToChinese` 语义不变(返回当前语言的动作显示名),全部调用点(`HelpContent`、设置下拉、`sync_docs.py` 的中文 README 表格)零改动;README 按键表是**文档**,不随程序语言切换(维持中文,单独维护)。
- 文档链:README(语言设置说明)、架构 spec/CLAUDE.md(Translations 子系统)、user-docs 同步说明。
- **明确不在本期**:第三种语言、右到左布局、帮助屏多语言排版差异调整、安装器语言包扩展。

**记录的假设**(实施前可推翻):技术路线为运行时查表而非 Qt tr/QTranslator 工具链(design 决策);设置对话框重开生效 vs 全量 retranslateUi 取轻者;英文文案由本期直接撰写(不做机翻占位)。

## Capabilities

### New Capabilities

- `ui-i18n`: 界面多语言。覆盖:语言选择与持久化(config 键 + 系统默认)、热切换的生效范围与边界、按键动作文案的双语呈现、缺失译文时的回退行为、新增语言的扩展方式。

### Modified Capabilities

- (无 —— `virtual-keyboard`/`config-profiles`/`live-help-screen` 的行为契约不变;它们的文案呈现经翻译表获得当前语言。)

## Impact

- **新增**:`src/core/Translations.{h,cpp}`(src_core,含英文表)、`tests/test_translations.cpp`。
- **修改**:`src/core/Types.{h,cpp}`(`actionToChinese` 内部接表)、`src/ui/SettingsDialog.{h,cpp}`(语言下拉 + 切换提示)、`src/ui/SystemTray.cpp`、`src/ui/HelpContent.cpp`、`src/ui/OsdOverlay.cpp`、`src/app/Application.cpp`(configChanged 接语言 + 确认框 tr 化)、`src/input/KeyboardController.cpp`(如有 UI 文案)、`config/default_config.json`(`ui.language`,纯附加不 bump)、`README_CN`→`README.md` 说明、架构文档。
- **风险**:英文表遗漏条目(fail-safe 回退中文,体验降级不出错——测试锁定核心条目覆盖);翻译表 key 与原文漂移(新增中文文案忘加英文 → 回退中文,由"表完整性测试"拦截);语言切换时已打开的设置对话框文案半新半旧(以"重开生效"为界,spec 明确该边界)。
