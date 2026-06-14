#include "GamepadPoller.h"
#include <QThread>
#include <QDebug>

GamepadPoller::PollThread::PollThread(GamepadPoller* parent)
    : m_parent(parent)
{
}

void GamepadPoller::PollThread::run() {
    while (!isInterruptionRequested()) {
        m_parent->pollOnce();
        msleep(kGamepadPollIntervalMs);
    }
}

GamepadPoller::GamepadPoller(QObject* parent)
    : QObject(parent)
    , m_thread(this)
{
}

GamepadPoller::~GamepadPoller() {
    stop();
}

void GamepadPoller::start() {
    m_thread.start();
}

void GamepadPoller::stop() {
    m_thread.requestInterruption();
    m_thread.quit();
    m_thread.wait();
}

static float applyDeadzone(float x, float y, float deadzone, float& outX, float& outY) {
    float mag = sqrtf(x * x + y * y);
    if (mag < deadzone) {
        outX = 0.0f;
        outY = 0.0f;
        return 0.0f;
    }
    float norm = (mag - deadzone) / (1.0f - deadzone);
    if (norm > 1.0f) norm = 1.0f;
    float nx = x / mag;
    float ny = y / mag;
    outX = nx * norm;
    outY = ny * norm;
    return norm;
}

void GamepadPoller::pollOnce() {
    for (int i = 0; i < 4; ++i) {
        XINPUT_STATE xState;
        bool connected = m_xinput.getState(i, &xState);

        GamepadState state;
        state.connected = connected;

        if (connected) {
            float lx = xState.Gamepad.sThumbLX / 32767.0f;
            float ly = xState.Gamepad.sThumbLY / 32767.0f;
            float rx = xState.Gamepad.sThumbRX / 32767.0f;
            float ry = xState.Gamepad.sThumbRY / 32767.0f;

            float deadzone = 7849.0f / 32767.0f;
            applyDeadzone(lx, ly, deadzone, state.leftX, state.leftY);
            applyDeadzone(rx, ry, deadzone, state.rightX, state.rightY);

            state.leftTrigger = xState.Gamepad.bLeftTrigger / 255.0f;
            state.rightTrigger = xState.Gamepad.bRightTrigger / 255.0f;
            state.buttons = xState.Gamepad.wButtons;
        }

        uint16_t prevButtons = m_prevState[i].buttons;
        bool wasConnected = m_prevState[i].connected;
        state.prevButtons = prevButtons;
        m_prevState[i] = state;

        if (connected && !wasConnected) {
            emit gamepadConnected(i);
        } else if (!connected && wasConnected) {
            emit gamepadDisconnected(i);
        }

        emit gamepadStateChanged(i, state);
    }
}
