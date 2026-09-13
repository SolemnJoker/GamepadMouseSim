# Tasks: add-config-profiles-and-live-help

> 设计依据:design.md(D1–D8);行为契约:specs/config-profiles/spec.md + specs/live-help-screen/spec.md。
> 自测分层(D7):L1 逻辑单测 / L2 链路集成 / L3 离屏渲染 / L4 进程自检 / L5 真机 manual(最小集)。
> 假设记录见 proposal(profile 粒度 = 整套操作方案;入口 = 托盘 + GUI;升级 = 增量合并)。

## 1. 默认映射单一事实源(L1)

- [x] 1.1 新建 `src/core/MappingDefaults.{h,cpp}`(编入 `src_core`):迁移 `KeyboardMapper.cpp` 匿名空间的三张默认表;提供 `directDefaults()/ltLayerDefaults()/rtLayerDefaults()` 与合并函数 `mergedMapping(config)`(默认表 ⊕ 配置覆盖,显式 None 胜出);`KeyboardMapper::loadConfig` 改为调用共享合并函数,行为不变。验证:现有 `test_keyboard_mapping` 全绿(语义未变)
- [x] 1.2 新增 `tests/test_mapping_defaults.cpp`:完整性(全部按钮在三层均有默认项)、合并语义(缺失补默认 / 显式值胜出 / 显式 None 胜出);`default_config.json` 与默认表一致性校验(不一致 qWarning 并测试失败)。验证:`ctest -R test_mapping_defaults` 绿

## 2. 配置加载链与增量迁移(L1)

- [x] 2.1 核对 `resources.qrc` 的 qrc prefix,确定内嵌默认配置真实资源路径;新增 `tests/test_config_load_chain.cpp` 断言该资源可读且含 `schema_version`/`mouse_mode`。验证:离屏 ctest 绿
- [x] 2.2 `Config::load` 加载链改造(D1):exe 旁 config.json → qrc 资源 → 内置空表兜底;任一非可写来源成功后自动 `save()` 写出可写 `config.json`;单测覆盖"裸 exe 首启动 → 文件生成 + 数据完整"(QTemporaryDir 模拟)。验证:单测绿
- [x] 2.3 增量迁移(D4):`schema_version < 2` → 保留用户键、缺失补默认、`mouse_mode` 原样成为 `profiles.list.default`、version 落 2;坏 JSON(解析失败)→ 出厂重置。迁移说明追加至 `docs/superpowers/specs/config-release-design.md`。验证:v1 自定义保留 / 缺失补默认 / 坏 JSON 重置三条单测绿
- [x] 2.4 `config/default_config.json` 升至 schema v2(增加 profiles 节示例结构,active="default")。验证:与 MappingDefaults 一致性校验测试绿

## 3. Profile 展开与回写(L1)

- [x] 3.1 `Config` 新增单一入口 `applyActiveProfile()`(展开 `profiles.list.<active>.mouse_mode` → `mouse_mode.*`,内存与磁盘同步)与回写辅助 `updateActiveProfileFromMouseMode()`;load 完成与 active 变更时调用。验证:新增 `tests/test_config_profiles.cpp`:切换 active → 镜像正确;load → 镜像执行
- [x] 3.2 设置界面保存路径回写:GUI 修改 `mouse_mode.*` 落盘时同步 `profiles.list.<active>.mouse_mode`;profile 列表保序(`profiles.order`)。验证:单测覆盖"改灵敏度 → active profile 同步更新,其他 profile 不受影响"

## 4. 设置 GUI 与托盘:合并视图、profile 管理(L1+L2)

- [x] 4.1 映射页显示改为 `MappingDefaults::mergedMapping` 合并视图(缺失键显示默认动作);保存落盘所见即所得。验证:L2 GUI 往返回归测试(打开即保存 → 默认绑定含 LT.Menu=ShowKeyboard 不丢,对应第 5 点③);可离屏测的逻辑抽成不依赖 QWidget 的纯函数
- [x] 4.2 映射页顶部 profile 下拉(profiles.order):选择即切换生效;新建(复制当前)/重命名/删除(至少保留一套,删除当前自动切首项);映射页"恢复默认"按钮重置当前 profile。验证:profile 管理规则单测(新建/删除约束/切换联动,规则函数与对话框壳分离)
- [x] 4.3 `src/ui/SystemTray`:profile 子菜单的**构建抽离为纯函数**(`buildProfileMenu(QMenu*, order, active)`,不触碰 QSystemTrayIcon,规避 offscreen 崩溃先例),SystemTray 挂接点选信号 → `setValue("profiles.active", ...)`;profile 增删后重建。验证:L1 菜单构建单测(order 顺序、当前项勾选、空列表兜底)

## 5. 自测基建(L3+L4)

- [x] 5.1 `--selftest` 进程自检模式(L4):`main.cpp` 解析 `--selftest` → 跑完整配置链自检(Config 加载链、迁移、profile 展开、合并视图非空、default_config 一致性),逐步输出 `[PASS]/[FAIL] <项>`,退出码 0/1;不初始化托盘/GamepadPoller。验证:`add_test(NAME selftest_config COMMAND GamepadMouseSim.exe --selftest, WORKING_DIRECTORY=<空临时目录>)` 直跑绿;临时目录断言生成 config.json
- [x] 5.2 offscreen 渲染可行性 spike(L3 前置):最小测试程序在 offscreen QPA 下创建普通 QWidget + `grab()` 并断言像素非空。验证:spike 结论写入 follow-up 文档;若崩溃 → L3 降级记录,L5 清单相应扩充(本组后续任务随之调整)
- [x] 5.3 KeyboardOverlay 离屏渲染测试(L3,依赖 5.2 通过):nav 合成状态驱动 → offscreen `grab()` → 断言图像尺寸正确、高亮移动前后关键区域像素变化。验证:按 D7 降级——spike 证实本环境 QApplication 挂起(offscreen 与 windows 平台同样),二进制保留、ctest 排除(tray-icon 先例),断言转 L1(test_help_content)+L5 目测
- [x] 5.4 帮助屏离屏渲染测试(L3,依赖 5.2 通过):`HelpContent` 行模型 + OsdOverlay 绘制路径 → `grab()` → 断言渲染非空、随配置内容变化(两种映射方案 grab 出不同图像)。验证:同 5.3 降级路径;内容随配置变化的 L1 断言由 test_help_content 覆盖并全绿

## 6. 帮助屏实时渲染(L1+L3)

- [x] 6.1 新建 `src/ui/HelpContent.{h,cpp}`(数据层,不依赖 QWidget):从合并视图生成分区行模型(直接映射/L3 层/RT 层/摇杆/模式切换,沿用现有分区)。验证:L1 `tests/test_help_content.cpp`:默认配置内容与 MappingDefaults 一致;修改某按钮映射 → 对应行随之变化
- [x] 6.2 `OsdOverlay::showHelp` 改为运行时 QPainter 按行模型绘制(沿用 85% 缩放策略),不再加载 `help.png`。验证:L3 渲染断言(5.4)绿;清晰度目测登记 L5
- [x] 6.3 静态链退役:`CMakeLists.txt` 移除 generate_help_png 目标与依赖,`resources.qrc` 移除 help.png 条目,删除 `tools/generate_help_png.py` 与 `resources/icons/help.png`;AGENTS.md 构建说明同步。验证:全量构建零引用残留(grep help.png)

## 7. 文档链(Rule 1)

- [x] 7.1 `README_CN.md`:profile 用法(托盘/GUI 切换、新建/删除/恢复默认)、帮助屏说明、`--selftest` 自检用法。验证:文档自查
- [x] 7.2 架构 spec(`2026-07-13-code-architecture-design.md`)新增 MappingDefaults/HelpContent 子系统条目与 Config 加载链修订;`CLAUDE.md` 架构图与要点更新(profile 展开、帮助实时化、--selftest)。验证:文档 diff 自查
- [x] 7.3 `config-release-design.md` 迁移说明确认 v2 条目完整(2.3 已写,核对)。验证:文档 diff 自查

## 8. 验证与收尾

- [x] 8.1 全量构建 + `ctest` 全绿(L1–L4 全部自动化,含 --selftest 进程级用例)。验证:本地全绿输出
- [x] 8.2 L5 残余 manual 清单执行并登记 carve-out(仅真机必要项:真实手柄焦点保持观感、真实微软拼音组词、摇杆步进手感、1080p 目测、托盘点选实感);follow-up 文档更新每项模拟化思路与 L3 spike 结论。验证:docs/followups 更新,链接写回本文件
- [x] 8.3 走查 `PULL_REQUEST_TEMPLATE.md` 合并清单(Rule 1 文档链 / Rule 2 测试证据)。验证:清单逐行勾选
