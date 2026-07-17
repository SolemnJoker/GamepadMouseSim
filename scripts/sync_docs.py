#!/usr/bin/env python3
"""
Sync README_CN.md button mapping table from actionToChinese().

Generates a fresh | 按键 | 功能 | table and replaces the section
between '## 基本操作' and the next heading in README_CN.md.

Usage: python scripts/sync_docs.py

Dependencies: None beyond Python stdlib.
"""

import re
import os

# Hard-coded mapping: ButtonAction enum name -> button name in README.
# This MUST be kept in sync with the actual button->action mapping in
# KeyboardMapper.cpp. If a new ButtonAction is added, add it here.
BUTTON_MAP = [
    ("A",   "MouseLeftClick"),
    ("B",   "MouseRightClick"),
    ("X",   "MouseMiddleClick"),
    ("LB",  "MouseLeftHold"),
    ("RB",  "MouseRightHold"),
    ("Y",   "KeyEnter"),
    ("L3",  "KeyEnter"),
    ("R3",  "KeyEscape"),
    ("View", "KeyTab"),
    ("Menu", "KeyWin"),
]

# Modifier-layer mappings (LT / RT)
LT_MAP = [
    ("LT+X",       "KeyAltTab"),
    ("LT+LB/RB",   "KeyShiftTab / Tab"),
    ("LT+Y",       "KeyAltF4"),
    ("LT+R3",      "ShowHelp"),
    ("LT+View",    ""),  # special: mode switch, not a ButtonAction
]

RT_MAP = [
    ("RT+A", "KeyCtrlA"),
    ("RT+B", "KeyCtrlC"),
    ("RT+X", "KeyCtrlX"),
    ("RT+Y", "KeyCtrlV"),
    ("RT+Dpad->/<-", "KeyCtrlTab / KeyCtrlShiftTab"),
    ("RT+Dpad/ /", "KeyPageUp / KeyPageDown"),
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
    "KeyWin":           "开始菜单",
    "ShowHelp":         "显示帮助",
    "VolumeUp":         "音量+",
    "VolumeDown":       "音量-",
    "VolumeMute":       "静音",
    "ScrollUp":         "向上滚动",
    "ScrollDown":       "向下滚动",
    "ScrollLeft":       "向左滚动",
    "ScrollRight":      "向右滚动",
    "Ctrl+Tab":         "切换标签",
    "Ctrl+Shift+Tab":   "切换标签（反向）",
}


def make_table(entries, label_col, action_col):
    """Generate a Markdown table from a list of (label, action) pairs."""
    lines = [
        "| {} | {} |".format(label_col, action_col),
        "|---|------|",
    ]
    for label, action in entries:
        if action == "":
            chinese = "长按1秒切换鼠标/默认模式"
        elif "/" in action:
            parts = [ACTION_CHINESE.get(a.strip(), a.strip()) for a in action.split("/")]
            chinese = " / ".join(parts)
        else:
            chinese = ACTION_CHINESE.get(action, action)
        lines.append("| {} | {} |".format(label, chinese))
    return "\n".join(lines) + "\n"


def sync_readme(readme_path):
    with open(readme_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Build the tables
    direct_table = make_table(BUTTON_MAP, "按键", "功能")
    lt_table = make_table(LT_MAP, "组合", "功能")
    rt_table = make_table(RT_MAP, "组合", "功能")

    # Construct the replacement section
    new_section = "## 🎮 基本操作\n\n"
    new_section += "| 按键 | 功能 |\n|------|------|\n" + "\n".join(
        "| {} | {} |".format(b, ACTION_CHINESE.get(a, a)) for b, a in BUTTON_MAP
    ) + "\n\n"
    new_section += "<sup>按键映射来源: core/Types.cpp actionToChinese()</sup>\n\n"
    new_section += "### 扳机组和层 (LT/RT)\n\n**按住 LT +**\n\n"
    new_section += "| 组合 | 功能 |\n|------|------|\n" + "\n".join(
        "| {} | {} |".format(b, ACTION_CHINESE.get(a, a)) for b, a in LT_MAP
    ) + "\n\n"
    new_section += "<sup>LT/RT 层修改器在 InputMapper.cpp 中定义</sup>"

    # Find the section to replace: between "## 🎮 基本操作" and next heading
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
    readme_path = os.path.join(repo_root, "README_CN.md")
    if not os.path.exists(readme_path):
        print("README_CN.md not found at", readme_path)
        return 1

    sync_readme(readme_path)
    return 0


if __name__ == "__main__":
    exit(main())