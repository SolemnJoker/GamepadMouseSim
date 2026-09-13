# Tasks: add-ui-i18n

> 设计依据:design.md(D1–D5);行为契约:specs/ui-i18n/spec.md。
> 假设记录见 proposal(查表路线 / 对话框重开生效 / 英文直接撰写)。

## 1. 翻译模块(核心)

- [x] 1.1 新建 `src/core/Translations.{h,cpp}`(编入 `src_core`):`tr(zh)`(查当前语言表,缺 key 回退原文)、`setLanguage/currentLanguage`、`resolveUiLanguage("system"/"zh"/"en")`(QLocale 解析,system → zh 系/其他 → en);英文表初版含托盘/帮助/OSD/确认框全部文案。验证:编译通过
- [x] 1.2 新增 `tests/test_translations.cpp`(L1):回退(未知 key → 原文)、语言三态解析、setLanguage 后 tr 输出变化;**全量源码扫描**:提取 `src/` 全部源文件中 `Translations::tr("...")` 字面参数,断言每条都在英文表内(设计 D4,2026-09-13 评审升级;豁免条目显式登记并注释)。验证:`ctest -R test_translations` 绿;故意删一条英文表条目 → 测试变红
- [x] 1.3 `Types.cpp`:`actionToChinese` 38 条文案改经 `Translations::tr` 返回当前语言显示名(函数名保留,注释说明);单测断言 zh/en 双语输出。验证:`test_action_to_chinese`(原中文断言改为"非空且非默认")与 1.2 双语断言绿

## 2. 调用点接入与语言切换

- [x] 2.1 `Application`:启动与 `configChanged` 中解析 `ui.language` 并 `Translations::setLanguage`;托盘"恢复默认配置"确认框(3 条文案)tr 化;configChanged 时托盘**全菜单**重建(现有 profile 子菜单重建扩展为整菜单,含模式/锁定/暂停文案)。验证:单测(解析/采集逻辑)+ 托盘重建逻辑离屏断言
- [x] 2.2 `SystemTray`(9 条)、`HelpContent`(12 条)、`OsdOverlay`(3 条)、`KeyboardController` 的 UI 可见文案(若有)全部 `Translations::tr` 化(日志不译)。验证:L1(test_help_content 断言调整为双语参数化)+ 编译
- [x] 2.3 `SettingsDialog`:常规页新增"界面语言"下拉(跟随系统/中文/English);选择即写 `ui.language`;语言变更时对话框内显示提示"语言将在重新打开设置后完全生效";74 条文案全量 `tr` 化。验证:语言键读写单测;对话框交互登记 L5
- [x] 2.4 `config/default_config.json` 新增 `"ui": { "language": "system" }`(纯附加,schema 不 bump);迁移说明追加条目。验证:与既有配置合并测试绿(旧配置缺 `ui` 节回退 "system")

## 3. 文档链(Rule 1)

- [x] 3.1 `README.md`:配置节增加界面语言说明(跟随系统/切换入口/生效范围);`docs/superpowers/specs/2026-07-13-code-architecture-design.md` 与 `CLAUDE.md`:Translations 子系统入图(core/ 级,src_core)。验证:文档 diff 自查
- [x] 3.2 `docs/superpowers/specs/config-release-design.md` 迁移说明:`ui.language` 附加条目。验证:文档 diff 自查

## 4. 验证与收尾

- [x] 4.1 全量构建 + `ctest` 全绿(新增 test_translations / test_action_to_chinese 更新)。验证:本地全绿
- [x] 4.2 L5 manual smoke(登记 followups):英文界面托盘/设置/帮助屏/OSD 的呈现与排版(英文比中文短,重点检查托盘提示与帮助屏三列布局不溢出);设置对话框重开生效边界。验证:docs/followups 更新(2026-09-13 条目;语言经设置下拉切换,无需 -D 参数)
- [x] 4.3 走查 `PULL_REQUEST_TEMPLATE.md` 清单(Rule 1 文档链 / Rule 2 测试证据)。验证:清单逐行勾选
