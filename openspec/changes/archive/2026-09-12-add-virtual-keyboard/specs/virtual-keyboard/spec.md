# Delta Spec: virtual-keyboard

## Purpose

让手柄用户在沙发上操控桌面 PC 时能够打字:提供手柄可导航的虚拟键盘 overlay,键位以真实按键注入当前持焦窗口,overlay 自身不抢焦点;通过标准键位使目标窗口的输入法照常完成中文组词,无需系统级输入法集成。

## ADDED Requirements

### Requirement: 键盘开合触发

系统 SHALL 提供"切换虚拟键盘"按钮动作(`ShowKeyboard`),用于显示/隐藏虚拟键盘 overlay。默认绑定 SHALL 为 LT 修饰层 + Menu 键;该动作 SHALL 作为一等 `ButtonAction` 出现在按键映射体系中,并因此可通过现有配置机制自定义绑定。

#### Scenario: 默认组合打开

- **WHEN** Mouse 模式下按住 LT 并按下 Menu
- **THEN** 虚拟键盘 overlay 显示在屏幕上

#### Scenario: 再次触发关闭

- **WHEN** 键盘已显示时再次触发同一动作
- **THEN** 键盘 overlay 隐藏

#### Scenario: 自定义绑定生效

- **WHEN** 用户在配置中将某个其他按钮(或层)映射为该切换动作
- **THEN** 该按钮/层触发同样的开合行为

### Requirement: 模式约束

虚拟键盘 SHALL 仅在 Mouse 模式下可触发和使用。Default 模式 SHALL 忽略切换动作,保持手柄信号纯透传。

#### Scenario: Default 模式下触发无效

- **WHEN** Default 模式下按下 LT+Menu
- **THEN** 键盘 overlay 不显示,手柄信号原样透传

#### Scenario: 模式切换时键盘自动关闭

- **WHEN** 键盘处于打开状态,用户切换到 Default 模式
- **THEN** 键盘 overlay 自动关闭

### Requirement: 手柄导航与输入路由

键盘打开期间,系统 SHALL 将手柄输入路由到键盘导航:摇杆/十字键在键位间移动高亮(支持四方向),A 键确认高亮键位,B 键等价退格。同一期间该手柄输入 SHALL NOT 驱动鼠标光标或触发常规映射。

#### Scenario: 方向导航

- **WHEN** 键盘打开时向右推摇杆(或按十字键右)
- **THEN** 高亮移动到右侧相邻键位

#### Scenario: 打开期间光标静止

- **WHEN** 键盘打开时移动摇杆
- **THEN** 鼠标光标不移动

#### Scenario: 关闭后恢复路由

- **WHEN** 键盘关闭后移动摇杆
- **THEN** 摇杆恢复正常驱动鼠标光标

### Requirement: 焦点纪律

键盘 overlay SHALL 以不激活窗口的方式显示和交互;从打开到使用的全程,系统前台窗口焦点 SHALL 保持停留在用户打开键盘前所在的目标窗口。

#### Scenario: 打开不改变前台窗口

- **WHEN** 用户正在某应用输入框中触发键盘切换动作
- **THEN** 该应用仍是前台焦点窗口,键盘 overlay 叠加显示

#### Scenario: 注入目标正确

- **WHEN** 键盘打开且用户确认字母键
- **THEN** 字符出现在打开键盘前持焦的窗口中

### Requirement: 键位注入

确认键位时,系统 SHALL 通过模拟键盘输入向当前前台窗口发送对应按键,行为等价于物理键盘敲击(含字符键、数字、空格、退格、Esc、回车、Shift 修饰)。

#### Scenario: 字符注入

- **WHEN** 前台为文本编辑器,用户导航到字母 "h" 并确认
- **THEN** 编辑器中出现 "h"

#### Scenario: 特殊键注入

- **WHEN** 用户确认空格/退格/Esc/回车键位
- **THEN** 前台窗口收到对应的标准按键效果

### Requirement: IME 透明性

键盘布局 SHALL 包含输入法组词所需的标准键位:数字行(0-9)、空格、退格、Esc。系统 SHALL NOT 对输入法做任何集成、拦截或状态切换——组词、候选与选字完全由目标窗口当前的输入法按物理键盘规则处理。

#### Scenario: 中文输入法组词

- **WHEN** 目标窗口开启微软拼音,用户依次输入 "n"、"i"、"2"
- **THEN** 与物理键盘行为一致:组词 "ni",数字 2 选中对应候选上屏

#### Scenario: 英文直通

- **WHEN** 目标窗口处于英文输入状态,用户输入字母与空格
- **THEN** 字母与空格直接上屏

#### Scenario: 取消组词

- **WHEN** 输入法组词中,用户确认 Esc 键位
- **THEN** 组词被取消(与物理键盘 Esc 行为一致)

### Requirement: 关闭途径

用户 SHALL 至少拥有两种关闭键盘的方式:再次触发切换动作;键盘布局上的专用关闭键。键盘关闭后,手柄输入路由 SHALL 立即恢复正常。

#### Scenario: 键盘上的关闭键

- **WHEN** 用户导航到键盘上的关闭键并确认
- **THEN** 键盘 overlay 关闭

#### Scenario: 切换动作关闭并恢复

- **WHEN** 键盘打开时再次触发切换动作
- **THEN** 键盘关闭,摇杆恢复驱动鼠标光标

### Requirement: 多手柄

任一已连接且处于 Mouse 模式的手柄 SHALL 都能打开键盘;键盘打开期间,任一已连接手柄的方向/确认输入 SHALL 作用于同一个键盘实例。

#### Scenario: 第二支手柄参与导航

- **WHEN** 手柄 1 打开键盘,手柄 2(已连接)推动摇杆
- **THEN** 同一键盘实例的高亮随之移动
