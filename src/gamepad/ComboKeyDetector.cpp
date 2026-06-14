#include "ComboKeyDetector.h"

ComboKeyDetector::ComboKeyDetector(QObject* parent)
    : QObject(parent)
{
}

void ComboKeyDetector::onGamepadState(const GamepadState& state) {
    if (!state.connected) {
        m_holding = false;
        return;
    }

    bool viewPressed = (state.buttons & XINPUT_GAMEPAD_BACK) != 0;
    bool menuPressed = (state.buttons & XINPUT_GAMEPAD_START) != 0;

    if (viewPressed && menuPressed) {
        if (!m_holding) {
            m_holding = true;
            m_holdTimer.start();
        } else if (m_holdTimer.elapsed() >= m_holdDurationMs) {
            m_holding = false;
            emit comboTriggered();
        }
    } else {
        m_holding = false;
    }
}
