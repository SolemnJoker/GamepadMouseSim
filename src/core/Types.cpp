#include "Types.h"

ButtonAction stringToAction(const QString& str) {
    if (str == "MouseLeftClick") return ButtonAction::MouseLeftClick;
    if (str == "MouseRightClick") return ButtonAction::MouseRightClick;
    if (str == "MouseMiddleClick") return ButtonAction::MouseMiddleClick;
    if (str == "MouseLeftHold") return ButtonAction::MouseLeftHold;
    if (str == "MouseRightHold") return ButtonAction::MouseRightHold;
    if (str == "Enter") return ButtonAction::KeyEnter;
    if (str == "Escape") return ButtonAction::KeyEscape;
    if (str == "Tab") return ButtonAction::KeyTab;
    if (str == "Backspace") return ButtonAction::KeyBackspace;
    if (str == "Delete") return ButtonAction::KeyDelete;
    if (str == "Home") return ButtonAction::KeyHome;
    if (str == "End") return ButtonAction::KeyEnd;
    if (str == "PageUp") return ButtonAction::KeyPageUp;
    if (str == "PageDown") return ButtonAction::KeyPageDown;
    if (str == "ArrowUp") return ButtonAction::KeyArrowUp;
    if (str == "ArrowDown") return ButtonAction::KeyArrowDown;
    if (str == "ArrowLeft") return ButtonAction::KeyArrowLeft;
    if (str == "ArrowRight") return ButtonAction::KeyArrowRight;
    if (str == "ShiftTab") return ButtonAction::KeyShiftTab;
    if (str == "Alt+Tab") return ButtonAction::KeyAltTab;
    if (str == "Alt+F4") return ButtonAction::KeyAltF4;
    if (str == "Win+D") return ButtonAction::KeyWinD;
    if (str == "Ctrl+W") return ButtonAction::KeyCtrlW;
    if (str == "Ctrl+A") return ButtonAction::KeyCtrlA;
    if (str == "Ctrl+C") return ButtonAction::KeyCtrlC;
    if (str == "Ctrl+V") return ButtonAction::KeyCtrlV;
    if (str == "Ctrl+X") return ButtonAction::KeyCtrlX;
    if (str == "Ctrl+Z") return ButtonAction::KeyCtrlZ;
    if (str == "Ctrl+Shift+Z") return ButtonAction::KeyCtrlShiftZ;
    if (str == "Ctrl+S") return ButtonAction::KeyCtrlS;
    if (str == "Ctrl+Left") return ButtonAction::KeyCtrlLeft;
    if (str == "Ctrl+Right") return ButtonAction::KeyCtrlRight;
    if (str == "Ctrl+Tab") return ButtonAction::KeyCtrlTab;
    if (str == "Ctrl+Shift+Tab") return ButtonAction::KeyCtrlShiftTab;
    if (str == "F5") return ButtonAction::KeyF5;
    if (str == "MediaPreviousTrack") return ButtonAction::MediaPrevTrack;
    if (str == "MediaNextTrack") return ButtonAction::MediaNextTrack;
    if (str == "VolumeUp") return ButtonAction::VolumeUp;
    if (str == "VolumeDown") return ButtonAction::VolumeDown;
    if (str == "VolumeMute") return ButtonAction::VolumeMute;
    if (str == "Win") return ButtonAction::KeyWin;
    if (str == "ScrollUp") return ButtonAction::ScrollUp;
    if (str == "ScrollDown") return ButtonAction::ScrollDown;
    if (str == "ScrollLeft") return ButtonAction::ScrollLeft;
    if (str == "ScrollRight") return ButtonAction::ScrollRight;
    if (str == "ShowHelp") return ButtonAction::ShowHelp;
    return ButtonAction::None;
}

QString actionToString(ButtonAction action) {
    switch (action) {
    case ButtonAction::MouseLeftClick: return "MouseLeftClick";
    case ButtonAction::MouseRightClick: return "MouseRightClick";
    case ButtonAction::MouseMiddleClick: return "MouseMiddleClick";
    case ButtonAction::MouseLeftHold: return "MouseLeftHold";
    case ButtonAction::MouseRightHold: return "MouseRightHold";
    case ButtonAction::KeyEnter: return "Enter";
    case ButtonAction::KeyEscape: return "Escape";
    case ButtonAction::KeyTab: return "Tab";
    case ButtonAction::KeyBackspace: return "Backspace";
    case ButtonAction::KeyDelete: return "Delete";
    case ButtonAction::KeyHome: return "Home";
    case ButtonAction::KeyEnd: return "End";
    case ButtonAction::KeyPageUp: return "PageUp";
    case ButtonAction::KeyPageDown: return "PageDown";
    case ButtonAction::KeyArrowUp: return "ArrowUp";
    case ButtonAction::KeyArrowDown: return "ArrowDown";
    case ButtonAction::KeyArrowLeft: return "ArrowLeft";
    case ButtonAction::KeyArrowRight: return "ArrowRight";
    case ButtonAction::KeyShiftTab: return "ShiftTab";
    case ButtonAction::KeyAltTab: return "Alt+Tab";
    case ButtonAction::KeyAltF4: return "Alt+F4";
    case ButtonAction::KeyWinD: return "Win+D";
    case ButtonAction::KeyCtrlW: return "Ctrl+W";
    case ButtonAction::KeyCtrlA: return "Ctrl+A";
    case ButtonAction::KeyCtrlC: return "Ctrl+C";
    case ButtonAction::KeyCtrlV: return "Ctrl+V";
    case ButtonAction::KeyCtrlX: return "Ctrl+X";
    case ButtonAction::KeyCtrlZ: return "Ctrl+Z";
    case ButtonAction::KeyCtrlShiftZ: return "Ctrl+Shift+Z";
    case ButtonAction::KeyCtrlS: return "Ctrl+S";
    case ButtonAction::KeyCtrlLeft: return "Ctrl+Left";
    case ButtonAction::KeyCtrlRight: return "Ctrl+Right";
    case ButtonAction::KeyCtrlTab: return "Ctrl+Tab";
    case ButtonAction::KeyCtrlShiftTab: return "Ctrl+Shift+Tab";
    case ButtonAction::KeyF5: return "F5";
    case ButtonAction::MediaPrevTrack: return "MediaPreviousTrack";
    case ButtonAction::MediaNextTrack: return "MediaNextTrack";
    case ButtonAction::VolumeUp: return "VolumeUp";
    case ButtonAction::VolumeDown: return "VolumeDown";
    case ButtonAction::VolumeMute: return "VolumeMute";
    case ButtonAction::KeyWin: return "Win";
    case ButtonAction::ScrollUp: return "ScrollUp";
    case ButtonAction::ScrollDown: return "ScrollDown";
    case ButtonAction::ScrollLeft: return "ScrollLeft";
    case ButtonAction::ScrollRight: return "ScrollRight";
    case ButtonAction::ShowHelp: return "ShowHelp";
    default: return "None";
    }
}

QString actionToChinese(ButtonAction action) {
    switch (action) {
    case ButtonAction::MouseLeftClick: return "左键单击";
    case ButtonAction::MouseRightClick: return "右键单击";
    case ButtonAction::MouseMiddleClick: return "中键单击";
    case ButtonAction::MouseLeftHold: return "左键按住";
    case ButtonAction::MouseRightHold: return "右键按住";
    case ButtonAction::KeyEnter: return "回车";
    case ButtonAction::KeyEscape: return "退出";
    case ButtonAction::KeyTab: return "Tab";
    case ButtonAction::KeyBackspace: return "退格";
    case ButtonAction::KeyDelete: return "删除";
    case ButtonAction::KeyHome: return "Home";
    case ButtonAction::KeyEnd: return "End";
    case ButtonAction::KeyPageUp: return "上翻页";
    case ButtonAction::KeyPageDown: return "下翻页";
    case ButtonAction::KeyArrowUp: return "方向↑";
    case ButtonAction::KeyArrowDown: return "方向↓";
    case ButtonAction::KeyArrowLeft: return "方向←";
    case ButtonAction::KeyArrowRight: return "方向→";
    case ButtonAction::KeyShiftTab: return "Shift+Tab";
    case ButtonAction::KeyAltTab: return "Alt+Tab";
    case ButtonAction::KeyAltF4: return "Alt+F4";
    case ButtonAction::KeyWinD: return "显示桌面";
    case ButtonAction::KeyCtrlW: return "关闭标签";
    case ButtonAction::KeyCtrlA: return "全选";
    case ButtonAction::KeyCtrlC: return "复制";
    case ButtonAction::KeyCtrlV: return "粘贴";
    case ButtonAction::KeyCtrlX: return "剪切";
    case ButtonAction::KeyCtrlZ: return "撤销";
    case ButtonAction::KeyCtrlShiftZ: return "重做";
    case ButtonAction::KeyCtrlS: return "保存";
    case ButtonAction::KeyCtrlLeft: return "Ctrl+←";
    case ButtonAction::KeyCtrlRight: return "Ctrl+→";
    case ButtonAction::KeyCtrlTab: return "Ctrl+Tab";
    case ButtonAction::KeyCtrlShiftTab: return "Ctrl+Shift+Tab";
    case ButtonAction::KeyF5: return "刷新";
    case ButtonAction::MediaPrevTrack: return "上一曲";
    case ButtonAction::MediaNextTrack: return "下一曲";
    case ButtonAction::VolumeUp: return "音量+";
    case ButtonAction::VolumeDown: return "音量-";
    case ButtonAction::VolumeMute: return "静音";
    case ButtonAction::KeyWin: return "开始菜单";
    case ButtonAction::ScrollUp: return "向上滚动";
    case ButtonAction::ScrollDown: return "向下滚动";
    case ButtonAction::ScrollLeft: return "向左滚动";
    case ButtonAction::ScrollRight: return "向右滚动";
    case ButtonAction::ShowHelp: return "显示帮助";
    default: return "无";
    }
}

QString buttonBitToName(uint16_t bit) {
    if (bit & XINPUT_GAMEPAD_A) return "A";
    if (bit & XINPUT_GAMEPAD_B) return "B";
    if (bit & XINPUT_GAMEPAD_X) return "X";
    if (bit & XINPUT_GAMEPAD_Y) return "Y";
    if (bit & XINPUT_GAMEPAD_DPAD_UP) return "DpadUp";
    if (bit & XINPUT_GAMEPAD_DPAD_DOWN) return "DpadDown";
    if (bit & XINPUT_GAMEPAD_DPAD_LEFT) return "DpadLeft";
    if (bit & XINPUT_GAMEPAD_DPAD_RIGHT) return "DpadRight";
    if (bit & XINPUT_GAMEPAD_LEFT_SHOULDER) return "LB";
    if (bit & XINPUT_GAMEPAD_RIGHT_SHOULDER) return "RB";
    if (bit & XINPUT_GAMEPAD_LEFT_THUMB) return "L3";
    if (bit & XINPUT_GAMEPAD_RIGHT_THUMB) return "R3";
    if (bit & XINPUT_GAMEPAD_BACK) return "View";
    if (bit & XINPUT_GAMEPAD_START) return "Menu";
    if (bit & XINPUT_TRIGGER_LEFT) return "LT";
    if (bit & XINPUT_TRIGGER_RIGHT) return "RT";
    return "";
}
