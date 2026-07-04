#!/usr/bin/env python3
"""
Pre-generate the help screen image for GamepadMouseSim.

Renders a fixed 1920x1080 PNG that the OsdOverlay loads at runtime,
completely avoiding any DPI-dependent layout issues.

Re-run this script whenever the help content or layout changes:
    python tools/generate_help_png.py

Output: resources/icons/help.png
"""

from PIL import Image, ImageDraw, ImageFont
import os
import sys

# ── Configuration ──────────────────────────────────────────────────────────
IMG_W, IMG_H = 1920, 1080
BG_COLOR = (0, 0, 0, 230)        # semi-transparent black
FONT_PATH = "msyh.ttc"           # Microsoft YaHei
TITLE_COLOR = (100, 200, 255)    # light blue
SECTION_COLOR = (255, 200, 100)  # gold
TEXT_COLOR = (255, 255, 255)     # white
ARROW_COLOR = (180, 180, 180)    # grey arrow

FONT_SIZE_TITLE = 48
FONT_SIZE_SECTION = 40
FONT_SIZE_TEXT = 34
FONT_SIZE_SMALL = 30

MARGIN = 60
LINE_GAP = 10
ARROW = "\u2192"  # →

# ── Help content ───────────────────────────────────────────────────────────
# Each section is a list of (button_label, action_text) or a string.
# Strings starting with "@" are section headers; empty string "" is a spacer.
HELP_SECTIONS = [
    # Each entry is either:
    #   a dict with a special key (@title or @section), or
    #   a tuple (button_label, action_text), or
    #   an empty string "" for spacer
    {"@title": "手柄鼠标模拟器"},
    "",

    # Direct mapping
    {"@section": "直接映射"},
    ("A", "左键单击"),
    ("B", "右键单击"),
    ("X", "中键单击"),
    ("Y", "回车"),
    ("LB", "左键按住"),
    ("RB", "右键按住"),
    ("View", "退出"),
    ("Menu", "Tab"),
    ("DpadUp", "方向↑"),
    ("DpadDown", "方向↓"),
    ("DpadLeft", "方向←"),
    ("DpadRight", "方向→"),
    ("LT", "向上滚动"),
    "",

    # L3 layer
    {"@section": "L3层 (按住L3)"},
    ("L3+A", "左键单击"),
    ("L3+B", "右键单击"),
    ("L3+X", "中键单击"),
    ("L3+Y", "回车"),
    ("L3+LB", "上一曲"),
    ("L3+RB", "下一曲"),
    ("L3+View", "切换模式"),
    ("L3+R3", "显示帮助"),
    "",

    # RT layer
    {"@section": "RT层 (按住RT)"},
    ("RT+A", "全选"),
    ("RT+B", "复制"),
    ("RT+X", "剪切"),
    ("RT+Y", "粘贴"),
    ("RT+LB", "音量+"),
    ("RT+RB", "音量-"),
    ("RT+View", "关闭标签"),
    ("RT+Menu", "刷新"),
    ("RT+DpadUp", "Home"),
    ("RT+DpadDown", "End"),
    ("RT+DpadLeft", "上翻页"),
    ("RT+DpadRight", "显示桌面"),
    "",

    # Sticks
    {"@section": "摇杆"},
    ("左摇杆", "移动鼠标"),
    ("右摇杆", "滚动页面"),
    "",

    # Mode switch
    {"@section": "模式切换"},
    ("L3+View(长按1秒)", "切换模式"),
]


def try_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    """Try to load Microsoft YaHei at the given size."""
    names = ["msyh.ttc", "msyhbd.ttc", "simhei.ttf", "simsun.ttc"]
    if bold:
        names = ["msyhbd.ttc", "msyh.ttc", "simhei.ttf", "simsun.ttc"]
    for n in names:
        try:
            return ImageFont.truetype(n, size)
        except OSError:
            continue
    return ImageFont.load_default()


def visual_width(text: str, font: ImageFont.FreeTypeFont, draw: ImageDraw.ImageDraw) -> int:
    """Get the pixel width of text using the given font."""
    bbox = draw.textbbox((0, 0), text, font=font)
    return bbox[2] - bbox[0]


def main():
    img = Image.new("RGBA", (IMG_W, IMG_H), BG_COLOR)
    draw = ImageDraw.Draw(img)

    font_title = try_font(FONT_SIZE_TITLE, bold=True)
    font_section = try_font(FONT_SIZE_SECTION, bold=True)
    font_text = try_font(FONT_SIZE_TEXT)
    font_small = try_font(FONT_SIZE_SMALL)

    # Collect all rows with their type and render info
    rows = []  # list of (type, left_text, right_text)
    # type: "title", "section", "mapping", "spacer"

    for section in HELP_SECTIONS:
        if isinstance(section, dict):
            if "@title" in section:
                rows.append(("title", section["@title"], ""))
            elif "@section" in section:
                rows.append(("section", section["@section"], ""))
        elif isinstance(section, tuple):
            rows.append(("mapping", section[0], section[1]))
        elif section == "":
            rows.append(("spacer", "", ""))

    # Calculate line heights
    def row_height(rtype):
        if rtype == "title":
            return FONT_SIZE_TITLE + LINE_GAP + 10
        elif rtype == "section":
            return FONT_SIZE_SECTION + LINE_GAP + 8
        elif rtype == "spacer":
            return FONT_SIZE_TEXT // 2
        else:
            return FONT_SIZE_TEXT + LINE_GAP

    # Layout into columns
    avail_h = IMG_H - MARGIN * 2
    # First pass: compute total height to determine columns
    total_h = sum(row_height(r[0]) for r in rows)
    num_cols = max(1, (total_h + avail_h - 1) // avail_h)

    # Split rows across columns
    col_width = (IMG_W - MARGIN * 2) // num_cols

    # Measure max button label width for arrow alignment
    max_label_w = 0
    for r in rows:
        if r[0] == "mapping":
            w = visual_width(r[1], font_text, draw)
            if w > max_label_w:
                max_label_w = w

    arrow_x_offset = max_label_w + 30  # space between label and arrow

    # Draw
    col = 0
    y = MARGIN
    col_x = MARGIN

    for rtype, left, right in rows:
        rh = row_height(rtype)

        # Column break?
        if y + rh > IMG_H - MARGIN and col < num_cols - 1:
            col += 1
            y = MARGIN
            col_x = MARGIN + col * col_width

        if rtype == "title":
            draw.text((col_x, y), left, font=font_title, fill=TITLE_COLOR)
        elif rtype == "section":
            draw.text((col_x, y), left, font=font_section, fill=SECTION_COLOR)
        elif rtype == "mapping":
            # Button label
            draw.text((col_x, y), left, font=font_text, fill=TEXT_COLOR)
            # Arrow
            ax = col_x + arrow_x_offset
            draw.text((ax, y), ARROW, font=font_text, fill=ARROW_COLOR)
            # Action text
            arrow_w = visual_width(ARROW + "  ", font_text, draw)
            draw.text((ax + arrow_w, y), right, font=font_text, fill=TEXT_COLOR)
        # spacer: skip

        y += rh

    # Output path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    out_path = os.path.join(project_dir, "resources", "icons", "help.png")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    img.save(out_path, "PNG")
    print(f"Generated: {out_path}  ({IMG_W}x{IMG_H})")


if __name__ == "__main__":
    main()
