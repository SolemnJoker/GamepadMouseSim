#!/usr/bin/env python3
"""
Sync README.md button mapping table from actionToChinese().

Generates fresh | 按键 | 功能 | tables and replaces the section
between '## 🎮 基本操作' and the next heading in README.md.

Usage: python scripts/sync_docs.py

Dependencies: None beyond Python stdlib.
"""

import os

# Must match MappingDefaults (single source of truth in src/core/MappingDefaults).
# "L3+" = the config LT layer, entered by holding L3 (left-stick click).
BUTTON_MAP = [
    ("A",    "MouseLeftClick"),
    ("B",    "MouseRightClick"),
    ("X",    "MouseMiddleClick"),
    ("LB",   "MouseLeftHold"),
    ("RB",   "MouseRightHold"),
    ("Y",    "KeyEnter"),
    ("R3",   "KeyEscape"),
    ("View", "KeyTab"),
    ("Menu", "KeyWin"),
]

LT_MAP = [
    ("L3+X",        "KeyAltTab"),
    ("L3+LB",       "KeyShiftTab"),
    ("L3+RB",       "KeyTab"),
    ("L3+A",        "KeyEnter"),
    ("L3+B",        "KeyEscape"),
    ("L3+Y",        "KeyAltF4"),
    ("L3+DpadUp",   "VolumeUp"),
    ("L3+DpadDown", "VolumeDown"),
    ("L3+DpadLeft", "VolumeMute"),
    ("L3+R3",       "ShowHelp"),
    ("L3+Menu",     "ShowKeyboard"),
]

RT_MAP = [
    ("RT+A",        "KeyCtrlA"),
    ("RT+B",        "KeyCtrlC"),
    ("RT+X",        "KeyCtrlX"),
    ("RT+Y",        "KeyCtrlV"),
    ("RT+DpadUp",   "KeyPageUp"),
    ("RT+DpadDown", "KeyPageDown"),
    ("RT+DpadLeft", "KeyCtrlShiftTab"),
    ("RT+RB",       "KeyCtrlS"),
    ("RT+L3",       "KeyWinD"),
    ("RT+R3",       "KeyCtrlZ"),
]

# Chinese label lookup (must match actionToChinese in Types.cpp)
ACTION_CHINESE = {
    "MouseLeftClick":   "鼠标左键",
    "MouseRightClick":  "鼠标右键",
    "MouseMiddleClick": "鼠标中键",
    "MouseLeftHold":    "按住左键（配合摇杆拖拽）",
    "MouseRightHold":   "按住右键（配合摇杆拖拽）",
    "KeyEnter":         "回车",
    "KeyEscape":        "退出",
    "KeyTab":           "Tab",
    "KeyShiftTab":      "Shift+Tab",
    "KeyAltTab":        "Alt+Tab",
    "KeyAltF4":         "关闭窗口",
    "KeyPageUp":        "上翻页",
    "KeyPageDown":      "下翻页",
    "KeyCtrlA":         "全选",
    "KeyCtrlC":         "复制",
    "KeyCtrlX":         "剪切",
    "KeyCtrlV":         "粘贴",
    "KeyCtrlS":         "保存",
    "KeyCtrlZ":         "撤销",
    "KeyCtrlShiftTab":  "关闭标签（反向）",
    "KeyWinD":          "显示桌面",
    "KeyWin":           "开始菜单",
    "ShowHelp":         "显示帮助",
    "ShowKeyboard":     "虚拟键盘",
    "VolumeUp":         "音量+",
    "VolumeDown":       "音量-",
    "VolumeMute":       "静音",
}


def make_table(entries, label_col, action_col):
    """Generate a Markdown table from a list of (label, action) pairs."""
    lines = [
        "| {} | {} |".format(label_col, action_col),
        "|---|------|",
    ]
    for label, action in entries:
        lines.append("| {} | {} |".format(label, ACTION_CHINESE.get(action, action)))
    return "\n".join(lines) + "\n"


def sync_readme(readme_path):
    with open(readme_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Construct the replacement section
    new_section = "## 🎮 基本操作\n\n"
    new_section += make_table(BUTTON_MAP, "按键", "功能") + "\n"
    new_section += "> L3 = 左摇杆按下。完整按键表见程序内帮助屏（L3+R3）——"
    new_section += "它永远与当前生效映射一致。\n\n"
    new_section += "### L3 层（按住 L3）\n\n"
    new_section += make_table(LT_MAP, "组合", "功能") + "\n"
    new_section += "### RT 层（按住 RT）\n\n"
    new_section += make_table(RT_MAP, "组合", "功能") + "\n"
    new_section += "<sup>表格由 scripts/sync_docs.py 从按键映射规范自动生成，请勿手工编辑。</sup>\n"

    # Replace between "## 🎮 基本操作" and the next heading
    start = content.find("## 🎮 基本操作")
    if start < 0:
        print("ERROR: Could not find '## 🎮 基本操作' in", readme_path)
        return False

    next_heading = content.find("\n## ", start + 2)
    if next_heading < 0:
        next_heading = len(content)

    old_section = content[start:next_heading]
    content = content.replace(old_section, new_section)

    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(content)

    print("Updated", readme_path)
    return True


def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    readme_path = os.path.join(repo_root, "README.md")
    if not os.path.exists(readme_path):
        print("README.md not found at", readme_path)
        return 1

    if not sync_readme(readme_path):
        return 1
    return 0


if __name__ == "__main__":
    exit(main())
