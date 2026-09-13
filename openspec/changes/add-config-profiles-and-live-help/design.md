# Design: add-config-profiles-and-live-help

## Context

- 诊断实证(2026-09-13):exe 旁两级配置文件全缺失时,Config 内存为空;日志 `No config file found, using defaults`。运行时行为正常纯靠 `KeyboardMapper` 的 D8 代码默认表;GUI 读空显示"无"。
- qrc 已打包 `config/default_config.json`(`resources.qrc:4`),但 `Config::load` 的 fallback 只尝试文件系统路径,从不读 `:/` 资源。
- `Config::load` 对 `schema_version < current` 的处理是 `loadDefault()` **整体重置**。
- `SettingsDialog::saveValues` 全量覆盖 `button_mapping`/`modifier_mapping`;其显示视图不含默认表 → 保存会把默认绑定显式写成 None。
- 帮助图链:`tools/generate_help_png.py` 构建期生成 1920×1080 PNG → qrc → `OsdOverlay::showHelp` 加载;内容手写,与运行时无关。
- 现有热加载:Config 300ms 去抖广播 `configChanged`,订阅者重读(架构 spec §2.3)。

## Goals / Non-Goals

**Goals:**

- 任何部署形态下首启动即有完整配置;GUI/帮助/运行时三处同源一致。
- 多套操作方案(profile)的保存、切换(托盘 + GUI)、恢复默认。
- 升级不丢用户配置(增量合并)。
- 帮助屏实时反映当前生效映射;静态 help.png 构建链退役。

**Non-Goals:**

- 手柄快捷键循环切换 profile(留作后续 change)。
- Per-app 自动 profile 切换(依前台进程选方案)。
- 云同步/导入导出配置文件。
- 摇杆加速度曲线形状等新参数(仅沿用现有键)。

## Decisions

### D1 — 配置加载链:文件 → qrc → 内置

加载顺序:`<exeDir>/config.json`(可写)→ qrc 资源 `:/config/default_config.json` → 编译期内置默认(空 JSON + MappingDefaults 兜底,理论不可达)。成功后若加载自非可写来源,立即 `save()` 写出 `config.json`。修复"裸 exe 无配置"与"GUI 读空"两个断裂。实现时需核对 qrc prefix(`resources.qrc` 的 `<qresource prefix>`)确定真实资源路径,并在单测中用真实资源路径断言。

**备选**:CMake 构建后拷贝 default_config.json 到输出目录——只修构建机不修安装包,且仍缺"裸 exe"形态,拒绝为主方案(可作为安装器附加保障,非本期)。

### D2 — 默认映射单一事实源:`src/core/MappingDefaults`

把 `KeyboardMapper.cpp` 匿名空间的三张默认表提升为 `src/core/MappingDefaults.{h,cpp}`(纯数据 + 合并函数:`mergedMapping(config)` = 默认表 ⊕ 配置覆盖),编入 `src_core` 以便 GUI/帮助/运行时/测试四方共用。`KeyboardMapper::loadConfig` 改为调用共享合并函数(行为不变);`SettingsDialog` 显示与保存、帮助内容生成同源。`default_config.json` 由同一数据人工对齐,加载时校验一致性(不一致记 qWarning)。

**备选**:default_config.json 作为唯一源(运行时读文件)——裸 exe 形态下文件可能缺失,又回到加载链问题;代码内数据不依赖 IO,选它做源,JSON 做投影。

### D3 — Profile 数据模型与展开:active 镜像到 mouse_mode

schema v2 配置形态:

```json
"schema_version": 2,
"profiles": {
  "active": "default",
  "list": {
    "default": { "mouse_mode": { ...整套... } },
    "media":   { "mouse_mode": { ... } }
  }
}
```

- **展开机制**:Config 加载完成与 `profiles.active` 变更时,把 `profiles.list.<active>.mouse_mode` 镜像写入 `mouse_mode.*`(内存与磁盘同步);运行时子系统继续读 `mouse_mode.*`,**零改动、零感知**。
- **切换** = `setValue("profiles.active", name)`(单键写入)→ 展开函数执行 → 既有 `configChanged` 300ms 去抖广播热生效,符合架构 spec §2.3"Config 单桥"。
- **编辑** = 设置界面直接改 `mouse_mode.*`(现有行为)→ 保存时回写 `profiles.list.<active>.mouse_mode`,保持镜像一致。
- profile 列表保序:JSON object 无序,另存 `profiles.order` 数组维护显示顺序。

**备选**:运行时双路径读取(profiles 节优先)——侵入所有订阅者,违背"运行时零感知";拒绝。

### D4 — 升级策略:增量合并(替代整体重置)

`schema_version < 2` 的迁移:v1 配置的 `mouse_mode` 原样成为 `profiles.list.default`,补缺失键为默认(与 D2 同一合并函数),`schema_version=2` 落盘。坏 JSON(解析失败)才出厂重置。`config-release-design.md` 追加 v1→v2 迁移说明。

### D5 — 帮助屏实时渲染:数据层 + 绘制分离

新增 `src/ui/HelpContent`(数据层):从"config + MappingDefaults 合并视图"生成分区内容(直接映射 / L3 层 / RT 层 / 摇杆 / 模式切换等,沿用现 PNG 的分区结构),产出供绘制的行模型。`OsdOverlay::showHelp` 改为运行时用 QPainter 按行模型绘制(复用现有全屏 85% 缩放策略),不再加载 `help.png`。**帮助内容数据层不依赖 QWidget**,可离屏单测断言"配置改了 → 内容跟着变"。构建链 `generate_help_png` 目标、qrc 条目、`tools/generate_help_png.py` 退役(脚本保留为历史参考或删除,tasks 定夺:删除)。

**备选**:运行时调 Python 重新生成 PNG——部署依赖 Python,荒谬;逐次缓存 QPixmap——首版直接绘制即可,性能足够(帮助屏非高频)。

### D6 — 设置 GUI:合并视图显示 + 保存 + profile 管理

- 显示:所有映射下拉以 `MappingDefaults::mergedMapping(config)` 为视图——缺失键显示默认动作而非"无"(修复"显示空")。
- 保存:写回所见即所得的合并结果(显式落盘,行为与显示一致);由于视图含默认值,"打开即保存"不再抹掉绑定。
- Profile 管理:顶部下拉(profiles.order)选择生效项;"新建"复制当前方案;重命名/删除带约束(至少保留一套;删除当前项时自动切到剩余首项);"恢复默认"按钮把当前 profile 的映射重置为共享默认。
- 托盘:右键菜单新增 profile 子菜单(order 顺序 + 当前项勾选),点选即 `setValue("profiles.active", ...)`。

### D7 — 自测策略:五层自动化金字塔(Rule 2,模拟优先)

自测手段按自动化程度分层,manual 只收留"物理上必须真机"的残余:

| 层 | 手段 | 覆盖 |
|----|------|------|
| L1 逻辑单测 | 纯逻辑类离屏单测(src_core / 不建 QWidget) | MappingDefaults 完整性与合并、Config 迁移、profile 展开/回写、HelpContent 行模型、托盘菜单构建(抽离为纯 QMenu 组装函数) |
| L2 链路集成 | 合成 GamepadState / 合成 config 驱动多组件 | InputMapper→showKeyboardRequested→Controller→FakeInjector 全链、GUI 往返(打开即保存不丢绑定) |
| L3 离屏渲染断言 | offscreen QPA + `QWidget::grab()` 像素断言 | KeyboardOverlay(键位网格渲染、高亮移动前后像素变化)、帮助屏(内容渲染非空、分区布局) |
| L4 进程级自检 | 主程序新增 `--selftest` 模式:跑完整配置链(加载→迁移→profile 展开→合并视图),逐步输出 PASS/FAIL,以退出码结束;不初始化托盘/线程。ctest 直接 `add_test(COMMAND GamepadMouseSim.exe --selftest)` 且 WORKING_DIRECTORY 指向空临时目录 | "裸 exe 首启动自动生成配置"、部署形态自检、发布前一键冒烟 |
| L5 真机 manual(最小集) | 真手柄 + 真显示器 | 焦点保持观感、真实 IME 组词(系统黑盒)、摇杆手感步进感、1080p 目测清晰度 |

- **L3 前置验证**:仓库已知坑是 `QSystemTrayIcon` 在 offscreen 下栈溢出(tray-icon 测试被 ctest 排除),普通 `QWidget::grab()` 在 offscreen 未验证过——先做 spike 任务验证可行性;若崩溃,该层降级为 L1 数据断言 + L5 目测,结论记入 follow-up(按 tray-icon 先例可保留二进制但从 ctest 排除)。
- **L4 与 L1 的关系**:`--selftest` 的每个检查项调用与单测相同的函数(Config/MappingDefaults),不复制逻辑;它新增的价值是**进程级 + 真实部署路径**(exe 旁真实文件系统形态),这是单测 QTemporaryFile 覆盖不到的。
- 残余 L5 清单集中在 follow-up 文档,每项附模拟化思路;随 L3/L4 落地持续收窄。

**备选**:引入 UI 自动化框架(pywinauto/WinAppDriver)驱动真窗口——重依赖、脚本脆,且 Qt 自带 offscreen + grab 已覆盖本需求,拒绝。

### D8 — 文档链(Rule 1)

`README_CN.md`(profile 用法、帮助说明)、架构 spec(新增 MappingDefaults/HelpContent 子系统 + Config 加载链修订)+ CLAUDE.md 图、`config-release-design.md`(v2 迁移)、AGENTS.md 构建说明(移除 help.png 生成步骤)。

## Risks / Trade-offs

- [qrc 资源路径前缀与预期不符] → 实现第一步核对 prefix 并以单测断言资源可读(D7)。
- [连续快速切换 profile × 300ms 去抖] → 展开在 setValue 时同步执行(内存即时正确),广播仅影响通知时序;托盘菜单项点选天然低频。
- [托盘菜单需随 profile 增删重建] → SystemTray 增加"重建 profile 菜单"入口,由 configChanged 触发(与现有菜单构建方式一致)。
- [镜像一致性(config 与 mouse_mode 双处存在)] → 所有写入经 SettingsGUI/Application 的统一回写路径;`Config` 提供 `applyActiveProfile()` 单一入口,杜绝旁路写入;单测覆盖往返。
- [帮助屏高 DPI 清晰度] → 沿用现有"1920×1080 逻辑尺寸 + 比例缩放"策略;真机 smoke 登记。
- [删除 tools/generate_help_png.py 影响 AGENTS.md 构建说明] → 文档链任务包含;CMake 移除目标与依赖。

## Migration Plan

首启动自动迁移 v1→v2(增量合并 + profile 化),无手工步骤。回滚:程序回退到旧版本时,schema_version=2 的配置文件会被旧版判定"过旧→重置"(旧逻辑)——回滚会丢 profile 结构但 mouse_mode 值仍在默认内;在迁移说明中注明。

## Open Questions

- profile 是否需要"复制自其他 profile"而不只是复制当前——v1 先支持"复制当前",不够再加。
- 帮助屏是否同时展示 profile 名称——v1 不展示,保持内容与旧版结构对齐。
