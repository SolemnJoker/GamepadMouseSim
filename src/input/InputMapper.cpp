#include "InputMapper.h"
#include <QDebug>

InputMapper::InputMapper(Config* config, int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_controllerIndex(controllerIndex)
    , m_mouseMapper(config, this)
    , m_keyboardMapper(config, this)
{
    connect(&m_keyboardMapper, &KeyboardMapper::showHelpRequested,
            this, &InputMapper::showHelpRequested);
}

void InputMapper::onGamepadStateChanged(int controllerIndex, const GamepadState& state) {
    if (controllerIndex != m_controllerIndex) return;
    if (!state.connected) return;

    m_keyboardMapper.processTrigger(state.leftTrigger, state.rightTrigger,
                                    m_prevLeftTrigger, m_prevRightTrigger);
    m_prevLeftTrigger = state.leftTrigger;
    m_prevRightTrigger = state.rightTrigger;

    if (m_mode != GamepadMode::Mouse) {
        bool r3Pressed = (state.buttons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        bool r3WasPressed = (state.prevButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        if (r3Pressed || r3WasPressed) {
            m_keyboardMapper.processButton(XINPUT_GAMEPAD_RIGHT_THUMB, r3Pressed, state.prevButtons);
        }
        return;
    }

    m_mouseMapper.processLeftStick(state.leftX, state.leftY);
    m_mouseMapper.processRightStick(state.rightX, state.rightY);

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
    if (mode != GamepadMode::Mouse) {
        m_keyboardMapper.releaseModifiers();
    }
    qDebug() << "InputMapper Pad" << m_controllerIndex << "mode:" << (mode == GamepadMode::Mouse ? "Mouse" : "Default");
}

void InputMapper::onConfigChanged() {
    qDebug() << "InputMapper Pad" << m_controllerIndex << "config changed, reloading";
    m_mouseMapper.onConfigChanged();
    m_keyboardMapper.onConfigChanged();
}
