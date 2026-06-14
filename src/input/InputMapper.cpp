#include "InputMapper.h"
#include <QDebug>

InputMapper::InputMapper(Config* config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_mouseMapper(config, this)
    , m_keyboardMapper(config, this)
{
}

void InputMapper::onGamepadStateChanged(const GamepadState& state) {
    if (!state.connected || m_mode != GamepadMode::Mouse) return;

    m_mouseMapper.processLeftStick(state.leftX, state.leftY);
    m_mouseMapper.processRightStick(state.rightX, state.rightY);

    m_keyboardMapper.processTrigger(state.leftTrigger, state.rightTrigger,
                                    m_prevLeftTrigger, m_prevRightTrigger);
    m_prevLeftTrigger = state.leftTrigger;
    m_prevRightTrigger = state.rightTrigger;

    uint16_t buttonsToCheck[] = {
        XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
        XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
        XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
        XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START
    };

    for (uint16_t btn : buttonsToCheck) {
        bool isPressed = (state.buttons & btn) != 0;
        bool wasPressed = (state.prevButtons & btn) != 0;
        if (isPressed || wasPressed) {
            m_keyboardMapper.processButton(btn, isPressed, state.prevButtons);
        }
    }
}

void InputMapper::onModeChanged(GamepadMode mode) {
    m_mode = mode;
    qDebug() << "InputMapper mode:" << (mode == GamepadMode::Mouse ? "Mouse" : "Default");
}

void InputMapper::onConfigChanged() {
    qDebug() << "InputMapper config changed, reloading";
    m_mouseMapper.onConfigChanged();
    m_keyboardMapper.onConfigChanged();
}
