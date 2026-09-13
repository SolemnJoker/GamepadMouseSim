# Delta Spec: config-profiles

## Purpose

让用户保存多套操作方案并随时切换生效:统一配置的加载链与默认值来源,使设置界面、帮助屏与运行时行为读同一份数据;旧配置升级保留用户自定义,不再整体重置。

## ADDED Requirements

### Requirement: 配置加载链兜底

系统 SHALL 按以下顺序加载配置:exe 旁可写 `config.json` → 程序内置默认配置(qrc 资源)→ 若均不可用则使用编译期内置默认表。任一环节成功后,系统 SHALL 确保存在一份可写的 `config.json`(首用时自动写出),使后续修改持久化。

#### Scenario: 裸 exe 首次运行

- **WHEN** exe 旁无任何配置文件时启动程序
- **THEN** 程序以内置默认配置正常运行,并自动在 exe 旁生成完整 `config.json`

#### Scenario: 设置界面与运行时一致

- **WHEN** 程序以任何配置形态启动后打开设置界面的按键映射页
- **THEN** 各按钮显示的映射与运行时实际执行的映射一致(含默认值),不再显示整页"无"

### Requirement: 默认映射单一事实源

按键映射的内置默认值 SHALL 只存在一份共享定义;运行时兜底、设置界面显示、帮助屏内容 SHALL 全部从该定义读取。默认值定义 SHALL 与出厂 `default_config.json` 保持一致(同一来源生成或加载时校验)。

#### Scenario: 三处显示一致

- **WHEN** 配置中某按钮的映射键缺失
- **THEN** 运行时执行、设置界面显示、帮助屏内容三者均呈现同一默认动作

### Requirement: Profile 数据模型与生效

系统 SHALL 支持 `profiles` 配置节:每套 profile 包含整套操作方案(直接映射、LT/RT 修饰层映射、摇杆灵敏度/死区);`profiles.active` 指定当前生效的 profile,其内容 SHALL 在加载与切换时展开至现有 `mouse_mode.*` 读取路径——既有子系统(映射/摇杆/虚拟键盘等)SHALL 无需感知 profile 的存在。

#### Scenario: 切换后热生效

- **WHEN** 用户从托盘菜单或设置界面选择另一套 profile
- **THEN** 按键映射与摇杆参数立即按新方案生效(经配置热加载广播,无需重启)

#### Scenario: 运行时读取路径不变

- **WHEN** 任一 profile 处于生效状态
- **THEN** 映射/摇杆子系统读取的仍是 `mouse_mode.*` 路径,行为与切换前架构一致

### Requirement: Profile 管理入口

用户 SHALL 能通过托盘菜单查看 profile 列表并点选切换(当前项有标识);SHALL 能在设置界面选择、新建(复制当前)、重命名与删除 profile(至少保留一套)。设置界面 SHALL 提供"恢复默认"将当前 profile 的映射重置为共享默认值。

#### Scenario: 托盘切换

- **WHEN** 用户在托盘菜单选择另一套 profile
- **THEN** 该 profile 生效,菜单中当前项有选中标识

#### Scenario: 新建与删除

- **WHEN** 用户在设置界面以当前方案新建一套 profile 并删除另一套
- **THEN** 列表正确更新,生效方案不受误删影响(不可删除最后一套)

#### Scenario: 恢复默认

- **WHEN** 用户对当前 profile 点击"恢复默认"
- **THEN** 该 profile 的映射恢复为共享默认值,帮助屏与运行时同步反映

### Requirement: 设置界面保存不抹默认绑定

设置界面 SHALL 以"配置 + 共享默认表"的合并视图展示与保存映射:显示缺失键为默认动作;保存落盘的映射 SHALL 与界面所见一致。用户的既有自定义绑定 SHALL NOT 因打开并保存设置界面而丢失或被显式 None 覆盖为默认以外的值。

#### Scenario: 打开保存不丢绑定

- **WHEN** 用户未修改任何映射,直接打开设置并保存
- **THEN** 所有默认绑定(含 LT 层 Menu=ShowKeyboard 等)保持原样生效

### Requirement: 增量合并升级

配置 schema_version 升级 SHALL 采用增量合并:用户配置中已存在的键保留,缺失的键补共享默认值,补齐后写入新 schema_version。SHALL NOT 因版本号升级而整体重置用户配置;仅当配置文件无法解析(坏 JSON)时才回退到出厂默认。

#### Scenario: 旧版本升级保留自定义

- **WHEN** schema_version=1 的用户配置在 v2 程序中加载,其中某按钮映射被用户改为自定义值
- **THEN** 该自定义值保留,缺失键补默认,schema_version 更新为 2

#### Scenario: 坏 JSON 回退

- **WHEN** `config.json` 内容无法解析为 JSON
- **THEN** 程序以内置默认配置正常运行,不崩溃,且用户可经由重新生成获得可写配置
