#pragma once

#include <cstdint>
#include <QString>

enum class GamepadMode {
    Default,
    Mouse
};

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
    ScrollRight
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

constexpr uint16_t XINPUT_GAMEPAD_DPAD_UP = 0x0001;
constexpr uint16_t XINPUT_GAMEPAD_DPAD_DOWN = 0x0002;
constexpr uint16_t XINPUT_GAMEPAD_DPAD_LEFT = 0x0004;
constexpr uint16_t XINPUT_GAMEPAD_DPAD_RIGHT = 0x0008;
constexpr uint16_t XINPUT_GAMEPAD_START = 0x0010;
constexpr uint16_t XINPUT_GAMEPAD_BACK = 0x0020;
constexpr uint16_t XINPUT_GAMEPAD_LEFT_THUMB = 0x0040;
constexpr uint16_t XINPUT_GAMEPAD_RIGHT_THUMB = 0x0080;
constexpr uint16_t XINPUT_GAMEPAD_LEFT_SHOULDER = 0x0100;
constexpr uint16_t XINPUT_GAMEPAD_RIGHT_SHOULDER = 0x0200;
constexpr uint16_t XINPUT_GAMEPAD_A = 0x1000;
constexpr uint16_t XINPUT_GAMEPAD_B = 0x2000;
constexpr uint16_t XINPUT_GAMEPAD_X = 0x4000;
constexpr uint16_t XINPUT_GAMEPAD_Y = 0x8000;

constexpr uint16_t XINPUT_TRIGGER_LEFT = 0x100;
constexpr uint16_t XINPUT_TRIGGER_RIGHT = 0x200;

ButtonAction stringToAction(const QString& str);
QString actionToString(ButtonAction action);
QString buttonBitToName(uint16_t bit);
