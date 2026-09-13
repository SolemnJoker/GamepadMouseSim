# Follow-up: add-virtual-keyboard manual smoke → 模拟化

来源:openspec change `add-virtual-keyboard` tasks 7.2(AGENTS.md Rule 2 的
manual-smoke carve-out 登记)。以下步骤无法在 CI/离屏环境模拟,需真机手柄
一次性验证;每项都附了后续模拟化的思路。

## Manual smoke 清单(本次发布前执行一次)

1. **真实前台焦点保持** — 在记事本输入框中触发 L3+Menu,确认:
   键盘弹出且记事本仍是前台(继续打字落在记事本);
   debug.log 中出现 "Keyboard overlay shown; foreground window: ..."。
2. **真实微软拼音组词** — 目标窗口开启微软拼音,导航输入 n、i,确认
   候选框出现在目标窗口旁;导航到数字行确认 2,候选上屏。
3. **Esc 取消组词** — 组词中确认 Esc 键位,组词取消且无字符落盘。
4. **1080p 显示** — 主屏 1080p 下截图确认键位不重叠、高亮清晰。

## 模拟化方案(本 issue 的收尾标准)

| 步骤 | 模拟思路 |
|------|----------|
| 1 | KeyInjector 已是接口(FakeKeyInjector);补一个 ForegroundProbe 接口包装 GetForegroundWindow/GetWindowText,Fake 返回预置窗口 → controller/overlay 日志路径可单测 |
| 2/3 | IME 组词行为属系统黑盒,无法进程内模拟;替代方案:记录注入 VK 序列与"物理键盘等价性"契约测试(已有 test_keyboard_nav 覆盖),真机步骤降为抽样回归 |
| 4 | 渲染像素验证可用 QWidget::grab() 离屏截图 + 图像断言(需先验证 offscreen QPA 对本窗口类稳定,参考 tray-icon 测试的排除教训) |

---

## 2026-09-13 补充:add-config-profiles-and-live-help 的 L3 spike 结论

**结论:L3(离屏渲染断言)在本仓库 CI 环境不可用,已按 design D7 降级。**
现象:`QApplication`(QTEST_MAIN Widgets 版)在当前非交互 shell 下挂起
或 fast-fail(退出码 0xc0000409,与早前 QSystemTrayIcon offscreen 栈溢出的
错误码完全一致——诊断于 2026-09-13 双重证实);offscreen 与 windows 平台
插件同样。根源:无法连接交互窗口站。三个渲染测试二进制
(test_offscreen_render / test_keyboard_overlay_render / test_help_overlay_render)
保留可构建,ctest 排除(tray-icon 先例);在交互桌面会话可手动运行。

**模拟化路径**(收窄 L5):
- 在 CI 机跑交互式会话(auto-logon)后,三个渲染测试可直接手动运行;
- 帮助内容"随配置变化"的断言已在 L1(test_help_content)覆盖,
  渲染层残余仅"视觉清晰度/布局美观",本质需要人眼,长期保留 L5。

**当前 L5 真机清单**(本 change):
1. 托盘"操作方案"子菜单点选切换实感;
2. 设置界面 profile 新建/重命名/删除/恢复默认交互;
3. 帮助屏 1080p 显示清晰度与三列布局(内容正确性已由 L1 保证);
4. 虚拟键盘遗留项(前文 1–4)。

---

## 2026-09-13 补充:add-ui-i18n 的 L5 清单

英文界面的呈现无法完全自动化(内容正确性已由 test_translations 的
全源扫描 + 回退测试锁定),残余真机目测项:

1. 语言设为 English 后:托盘菜单/提示、设置对话框全部页签、帮助屏
   (`L3+R3`)、OSD、恢复默认确认框的英文呈现与排版(英文比中文短,
   重点看托盘提示与帮助屏三列布局不溢出);
2. 设置对话框内切换语言 → 提示文案出现 → 重开对话框后全部为新语言;
3. 跟随系统模式:在英文系统(或改系统语言)下首启动即为英文。
