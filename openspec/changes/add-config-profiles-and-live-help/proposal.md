# Proposal: add-config-profiles-and-live-help

## Why

用户报告的 5 个问题经诊断同根:**配置管线多处断裂,帮助提示与实际生效映射没有任何运行时关联**。

实证(2026-09-13,依据运行日志与代码):

1. **帮助提示与默认配置不一致**——帮助图内容是构建时脚本 `tools/generate_help_png.py` 手写的(旧映射,如 View="退出"、Menu="Tab"),与代码内置默认表、`default_config.json`、README 各说各话。
2. **改配置后帮助不同步**——帮助图是编译进 exe 的静态 PNG(`resources.qrc`),与运行时配置零关联。
3. **无多配置方案**——只有一份全局映射,无法为"桌面/媒体播放/浏览器"等场景保存并切换多套操作方案。
4. **设置 GUI 显示空**——exe 旁 `config.json` 与 `config/default_config.json` 都缺失(日志:`No config file found`),Config 内存为空;运行时行为正常纯靠 D8 代码默认表兜底,**GUI 显示与实际行为脱节**。
5. **配置管线不兼容**(逐条实证):
   - qrc 打包了 `default_config.json` 但加载链从不读 `:/` 资源——bundled 资源形同虚设;
   - schema 升级策略为 `version < current` 即**整体重置**,用户自定义全部丢失;
   - 设置 GUI 保存时全量覆盖映射表:GUI 显示视图不含默认表,保存会把默认绑定(如 LT 层 Menu=ShowKeyboard)显式写成 None **抹掉**——即用户打开一次设置并保存,本次虚拟键盘功能就失效;
   - 空 config 下 GUI 首次保存会写出一半缺键的 `config.json`。

## What Changes

- **配置加载链修复**:加载顺序改为 `<exeDir>/config.json` → qrc 内嵌默认配置(`:/config/default_config.json`)→ 自动写出可写的 `config.json`;保证任何部署形态下(绿色版/安装版/裸 exe)首次运行即有完整配置,GUI 与运行时读同一份数据。
- **默认映射单一事实源**:把 `KeyboardMapper` 的代码内置默认表提取为共享模块(如 `src/core/MappingDefaults`),设置 GUI 显示、帮助屏渲染、运行时兜底三者读同一份;`default_config.json` 与之对齐。
- **帮助屏实时化**:帮助 overlay 改为运行时按"当前生效映射"(config + 默认表合并视图)动态绘制;废除 `tools/generate_help_png.py` 构建链与 `help.png` 资源。配置热加载后,下一次呼出帮助即为最新映射。
- **多配置方案(profile)**:新增 `profiles` 配置节,每套 profile = 整套操作方案(按键映射 + LT/RT 修饰层 + 摇杆灵敏度/死区,即整个 `mouse_mode` 节);`profiles.active` 指定生效方案,切换时展开写入 `mouse_mode.*` 并经 `configChanged` 热广播——运行时读取路径零改动。入口:托盘菜单(点选切换)+ 设置 GUI(选择/新建/重命名/删除)。手柄快捷键循环切换列为非目标(可后续追加)。
- **升级策略改为增量合并**:`schema_version` 升至 2;旧配置迁移 = 保留用户已有键、缺失键补默认(与 D8 机制同源),仅坏 JSON(无法解析)才出厂重置;迁移时现有 `mouse_mode` 原样成为首个 profile("default")。
- **GUI"恢复默认"**:每个 profile 提供一键重置为本 profile 内的默认映射(用共享默认表)。
- 文档链:`README_CN.md`、架构 spec/CLAUDE.md、`config-release-design.md` 迁移说明、AGENTS.md 构建说明(help.png 生成步骤移除)。

**记录的假设**(提案时用户未逐项确认,按推荐项取,实施前可推翻):profile 粒度 = 整套操作方案;切换入口 = 托盘 + GUI 双入口(不含手柄快捷键);升级策略 = 增量合并。

## Capabilities

### New Capabilities

- `config-profiles`: 多套操作方案的存储、切换与升级合并。覆盖:profile 数据模型与 active 展开、托盘/GUI 切换入口、GUI 按合并视图显示与保存(不再抹默认绑定)、schema v2 增量迁移、配置加载链兜底(qrc bundled)。
- `live-help-screen`: 帮助屏按当前生效映射实时渲染。覆盖:内容与运行时配置一致、配置热加载后立即反映、替代构建时 help.png。

### Modified Capabilities

- (无 —— `virtual-keyboard` 等既有能力的运行时读取路径不变;映射数据来源扩展对它们透明。)

## Impact

- **修改**:`src/core/Config.{h,cpp}`(加载链、增量迁移、profile 展开)、`src/input/KeyboardMapper.cpp`(默认表提取至共享模块)、`src/ui/SettingsDialog.{h,cpp}`(合并视图显示、profile 管理、恢复默认)、`src/ui/SystemTray.{h,cpp}`(profile 菜单)、`src/ui/OsdOverlay.{h,cpp}`(帮助实时渲染)、`src/app/Application.{h,cpp}`(接线)、`resources/resources.qrc`(help.png 移除、默认配置确认)、`CMakeLists.txt`(generate_help_png 目标移除)、`config/default_config.json`(profiles 节、schema v2)。
- **新增**:`src/core/MappingDefaults.{h,cpp}`(共享默认表)、`src/ui/HelpContent`(或同类,帮助内容→绘制数据)、`tests/` 下 Config 迁移/展开/GUI 往返等测试。
- **移除**:`tools/generate_help_png.py` 退役(或保留为开发参考但退出构建)、`resources/icons/help.png`。
- **风险**:qrc 资源路径前缀需与打包路径核对;profile 展开与 300ms 热加载去抖的交互(连续切换);托盘菜单动态重建;帮助屏 1920×1080 固定尺寸渲染在高 DPI 下的清晰度(沿用现有缩放策略)。
