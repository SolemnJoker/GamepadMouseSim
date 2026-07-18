#pragma once

#include <QString>
#include <cstdint>

enum class GamepadMode { Default, Mouse };

enum class ButtonAction {
    None,
    MouseLeftClick,
    MouseRightClick,
    MouseMiddleClick,
    MouseLeftHold,
    MouseRightHold,
    KeyEnter,
    KeyEscape,
    KeyTab,
    KeyBackspace,
    KeyDelete,
    KeyHome,
    KeyEnd,
    KeyPageUp,
    KeyPageDown,
    KeyArrowUp,
    KeyArrowDown,
    KeyArrowLeft,
    KeyArrowRight,
    KeyShiftTab,
    KeyAltTab,
    KeyAltF4,
    KeyWinD,
    KeyCtrlW,
    KeyCtrlA,
    KeyCtrlC,
    KeyCtrlV,
    KeyCtrlX,
    KeyCtrlZ,
    KeyCtrlShiftZ,
    KeyCtrlS,
    KeyCtrlLeft,
    KeyCtrlRight,
    KeyCtrlTab,
    KeyCtrlShiftTab,
    KeyF5,
    MediaPrevTrack,
    MediaNextTrack,
    VolumeUp,
    VolumeDown,
    VolumeMute,
    KeyWin,
    ScrollUp,
    ScrollDown,
    ScrollLeft,
    ScrollRight,
    ShowHelp
};

struct GamepadState {
    bool connected = false;
    float leftX = 0.0f;
    float leftY = 0.0f;
    float rightX = 0.0f;
    float rightY = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    uint16_t buttons = 0;
    uint16_t prevButtons = 0;
};

constexpr float kDefaultSensitivityX = 1.0f;
constexpr float kDefaultSensitivityY = 1.0f;
constexpr float kDefaultDeadzone = 0.15f;
constexpr float kDefaultScrollSpeed = 1.0f;
constexpr int kDefaultPollingIntervalMs = 2000;
constexpr int kDefaultManualLockoutMs = 30000;
constexpr int kDefaultComboHoldMs = 1000;
constexpr int kGamepadPollHz = 60;
constexpr int kGamepadPollIntervalMs = 1000 / kGamepadPollHz;
constexpr int kMaxGamepads = 4;

// XInput raw-value normalization ranges (see src/win/XInputWrapper.h).
constexpr int kXInputThumbMax = 32767;   // signed thumb-stick saturation
constexpr int kXInputTriggerMax = 255;   // unsigned trigger saturation
constexpr int kXInputDeadzoneMax = 7849; // XInput recommended deadzone upper bound
constexpr int kMsPerSecond = 1000;       // seconds → milliseconds
constexpr int kCurrentConfigSchemaVersion = 1; // bump on breaking config changes

#ifndef XINPUT_GAMEPAD_DPAD_UP
constexpr uint16_t XINPUT_GAMEPAD_DPAD_UP = 0x0001;
#endif
#ifndef XINPUT_GAMEPAD_DPAD_DOWN
constexpr uint16_t XINPUT_GAMEPAD_DPAD_DOWN = 0x0002;
#endif
#ifndef XINPUT_GAMEPAD_DPAD_LEFT
constexpr uint16_t XINPUT_GAMEPAD_DPAD_LEFT = 0x0004;
#endif
#ifndef XINPUT_GAMEPAD_DPAD_RIGHT
constexpr uint16_t XINPUT_GAMEPAD_DPAD_RIGHT = 0x0008;
#endif
#ifndef XINPUT_GAMEPAD_START
constexpr uint16_t XINPUT_GAMEPAD_START = 0x0010;
#endif
#ifndef XINPUT_GAMEPAD_BACK
constexpr uint16_t XINPUT_GAMEPAD_BACK = 0x0020;
#endif
#ifndef XINPUT_GAMEPAD_LEFT_THUMB
constexpr uint16_t XINPUT_GAMEPAD_LEFT_THUMB = 0x0040;
#endif
#ifndef XINPUT_GAMEPAD_RIGHT_THUMB
constexpr uint16_t XINPUT_GAMEPAD_RIGHT_THUMB = 0x0080;
#endif
#ifndef XINPUT_GAMEPAD_LEFT_SHOULDER
constexpr uint16_t XINPUT_GAMEPAD_LEFT_SHOULDER = 0x0100;
#endif
#ifndef XINPUT_GAMEPAD_RIGHT_SHOULDER
constexpr uint16_t XINPUT_GAMEPAD_RIGHT_SHOULDER = 0x0200;
#endif
#ifndef XINPUT_GAMEPAD_A
constexpr uint16_t XINPUT_GAMEPAD_A = 0x1000;
#endif
#ifndef XINPUT_GAMEPAD_B
constexpr uint16_t XINPUT_GAMEPAD_B = 0x2000;
#endif
#ifndef XINPUT_GAMEPAD_X
constexpr uint16_t XINPUT_GAMEPAD_X = 0x4000;
#endif
#ifndef XINPUT_GAMEPAD_Y
constexpr uint16_t XINPUT_GAMEPAD_Y = 0x8000;
#endif

#ifndef XINPUT_TRIGGER_LEFT
constexpr uint16_t XINPUT_TRIGGER_LEFT = 0x100;
#endif
#ifndef XINPUT_TRIGGER_RIGHT
constexpr uint16_t XINPUT_TRIGGER_RIGHT = 0x200;
#endif

ButtonAction stringToAction(const QString& str);
QString actionToString(ButtonAction action);
QString actionToChinese(ButtonAction action);
QString buttonBitToName(uint16_t bit);
