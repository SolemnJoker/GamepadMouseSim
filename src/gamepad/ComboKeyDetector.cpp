#include "ComboKeyDetector.h"
#include <QDebug>

ComboKeyDetector::ComboKeyDetector(int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_controllerIndex(controllerIndex)
{
}

void ComboKeyDetector::onGamepadState(int controllerIndex, const GamepadState& state) {
    if (controllerIndex != m_controllerIndex) return;
    if (!state.connected) {
        m_holding = false;
        return;
    }

    bool l3Pressed = (state.buttons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    bool viewPressed = (state.buttons & XINPUT_GAMEPAD_BACK) != 0;

    if (l3Pressed && viewPressed) {
        if (!m_holding) {
            m_holding = true;
            m_holdTimer.start();
        } else if (m_holdTimer.elapsed() >= m_holdDurationMs) {
            m_holding = false;
            qDebug() << "Combo triggered on Pad" << m_controllerIndex;
            emit comboTriggered(m_controllerIndex);
        }
    } else {
        m_holding = false;
    }
}
