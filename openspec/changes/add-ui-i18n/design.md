# Design: add-ui-i18n

## Context

- 中文文案分布(proposal 盘点):`SettingsDialog` 74、`actionToChinese` 38、`HelpContent` 12、`SystemTray` 9、零散 9;`KeyboardOverlay` 键帽为字母/符号无需处理。
- 现有渲染节奏天然利于查表:托盘菜单在 configChanged 时重建、OSD/帮助屏**每次显示时生成**、`HelpContent` 是纯数据层、`KeyboardOverlay` 每帧 paint——只有 `SettingsDialog` 的控件文案在构造时固化。
- `actionToChinese` 是唯一的动作文案出口(调用点:`HelpContent`、设置下拉),`sync_docs.py` 生成的中文 README 表是**文档**不随程序语言变。
- 配置热加载链路(configChanged 广播)与 profile 切换模式(镜像/采集)已就绪,语言切换复用同一广播。

## Goals / Non-Goals

**Goals:**

- 中/英双语言;语言选择(config + 设置下拉)、持久化、默认跟随系统。
- 显示时查表的即时热切换(除设置对话框重建边界)。
- 缺失译文 fail-safe 回退中文;表完整性可测试。
- 新语言零调用点改动。

**Non-Goals:**

- Qt tr()/QTranslator/lupdate 工具链(见 D1)。
- README/用户手册的英文版(文档与 UI 语言分离)。
- 安装器语言扩展(isl 已双语)、第三语言、RTL。

## Decisions

### D1 — 运行时查表(中文原文为 key),不用 QTranslator

`src/core/Translations`(src_core):`tr(zh)` 在当前语言表中查 `zh → 英文`,缺 key 返回原文;`setLanguage/currentLanguage`;语言枚举 `zh`/`en`,`ui.language="system"` 在 Application 启动与 configChanged 时解析为具体语言(QLocale::system().name() 前缀)。

**为什么不用 Qt tr/QTranslator**:①源文本是中文,tr() 只是把硬编码换成 tr 包裹 + lupdate 全量扫描 + .ts 翻译 + lrelease + qrc 版本管理,两语言的工具链成本远超一张数据表;②本项目已有"数据表即事实源"的惯例(MappingDefaults),翻译表是同一模式的延伸;③显示时查表天然热切换,QTranslator 需要 LanguageChange 事件 + 每个组件 retranslateUi,与"设置对话框重建"的轻方案冲突。**备选已否决**:QTranslator(工具链重、动态切换复杂)、QString 单数全局表以英文为 key(要把 142 条中文改成英文 key,且回退目标应是中文原文)。

### D2 — 动作文案:`actionToChinese` 内部接表,调用点零改动

`actionToChinese(action)` 的 38 条中文文案全部进入翻译表(key=中文原文),函数体改为 `return Translations::tr(中文原文)`。英文环境下它返回英文动作名——`HelpContent`、设置下拉、`sync_docs.py`(生成中文 README,不经过程序运行时)**全部无需改动**。函数改名与否:保留现名(重命名会牵连 C2 已发布 spec/文档链,v1 用名实不完全符 + 注释说明,重命名留给后续 cleanup——已记 Open Questions)。

### D3 — 语言解析与热广播

- `ui.language` 取值 `"system"/"zh"/"en"`;`"system"` → `QLocale::system().name()` 前缀 `zh` → 中文,否则英文(英文兜底,因为英文表覆盖靠测试保证而中文是源)。
- Application 启动与 `configChanged` 中调用 `Translations::setLanguage(resolved)`;托盘在既有 configChanged 重建钩子中重建全部菜单(不只 profile 子菜单);OSD/帮助/键盘/确认框显示时查表即新语言。
- `SettingsDialog`:语言下拉写入 `ui.language` → configChanged → `setLanguage`;对话框若打开,弹出提示条(spec 场景"重新打开设置以更新语言")——实现:对话框保存语言后自我关闭并在托盘提示?**定稿**:对话框内切换语言下拉时,立即写入 config,并在对话框内显示一行提示标签"语言将在重新打开设置后完全生效";用户手动关闭重开。不做 retranslateUi(74 条文案的 retranslate 函数维护成本高于收益,记录为后续可选项)。

### D4 — 英文表与**全量源码扫描**完整性测试

- 英文表 = `Translations.cpp` 内一张 `QMap<QString, QString>`(key=中文原文,value=英文);按归属分组注释(托盘/动作/帮助/对话框/OSD)。
- 测试(`tests/test_translations.cpp`,src_core 离屏):①**全量扫描**——扫描 `src/` 全部源文件的 `Translations::tr("...")` 字面参数,断言每一条都在英文表内(2026-09-13 评审升级:原"闭合集合"方案漏掉 SettingsDialog 74 条,漂移防护从核心集合扩展为全量;中文原文 key 模式由此从"靠自觉"变为测试锁死);②回退——未知 key 返回原文;③语言解析——system/zh/en 三态;④动作双语——`actionToChinese` 在 en 下返回英文、zh 下返回中文。
- 漂移语义:改中文原文 → 扫描测试红(强制同步英文表);新增文案漏翻 → 回退中文显示(体验降级不出错),但扫描测试同样红 → **事实上全量强制**。若某条暂无法翻译,显式登记"豁免清单"并注释原因。

### D5 — 文案调用点改造清单(改法统一)

全部界面文案调用点改为 `Translations::tr(...)`:`SystemTray`(9)、`HelpContent`(12,含分区标题与固定行)、`OsdOverlay`(3)、`Application` 确认框(3)、`SettingsDialog`(74——本期只 tr 化其**静态外壳**亦可全量,任务里定:全量,机械替换)。`KeyboardController` 的 3 条为日志/OSD 文本,日志**不翻译**(qDebug 面向诊断),仅 UI 可见者翻译。

### D6 — 演进路径(面向未来语言与 UI 重构,2026-09-13 评审补充)

- **第三语言**:追加一张译文表 + `ui.language`/下拉登记,调用点零改动(spec 已定)。何时迁移到 ID 化 key(`tr("tray.exit")` 替代 `tr("退出")`):当出现**语境冲突**(同一中文词需按位置不同译)或语言数 ≥3 且翻译工作流需要独立管理时。迁移是**机械脚本替换**(全部 `tr("中文")` → `tr("id")`,调用点数量不变),表结构不变——演进面被 D5 的统一调用点收敛在低风险区。
- **UI 框架/界面重构**:`tr()` 是普通函数调用,重构组件照抄调用即可;若新框架需要文案数据(而非 C++ 调用),给 Translations 追加 `snapshot(lang)` 导出(QMap → JSON),纯数据模块一处实现——**v1 不实现**,但接口位置已预留(数据与访问天然分离)。
- **字体随语言**:帮助屏/OSD 硬编码微软雅黑对日文假名/韩文覆盖不全——第三语言落地时增加"字体族按当前语言选择"的小改动(见 Open Questions)。

## Risks / Trade-offs

- [英文表与中文原文漂移] → D4 闭合集合测试 + fail-safe 回退(显示中文不出错)。
- [设置对话框打开期间文案半旧] → spec 明确"重开生效"+ 对话框内提示条,v1 接受。
- [`actionToChinese` 名实不符(英文名返回英文)] → 保留函数名,v1 注释说明;重命名延后。
- [system locale 解析在多语言用户下的歧义] → 首选 UI 语言 locale(QLocale::system().uiLanguages() 首项),回落 name() 前缀。
- [SettingsDialog 74 条机械替换引入笔误] → 全量替换后构建 + 表覆盖测试 + 人工过一遍对话框(登记 L5)。

## Migration Plan

纯附加:新 config 键(`ui.language` 缺省 "system",缺失回退默认路径已有)、新模块、新测试。回滚 = 移除调用点替换即可(文案回到硬编码中文)。

## Open Questions

- 语言下拉是否提供"跟随系统"显式选项(推荐提供,默认选中)——实施时定,不影响 spec 行为。
- 英文动作文案的措辞标准(如"鼠标左键"= "Left click" vs "Mouse left click")——实施时按简洁优先统一,记录在英文表注释。
- **字体族随语言**(D6):帮助屏/OSD 的 Microsoft YaHei 对日韩字形覆盖不全,第三语言落地时引入"按语言选择字体族"配置——本期不动,防止范围蔓延。
