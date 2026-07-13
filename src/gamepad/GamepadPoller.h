#pragma once

#include <array>
#include <QObject>
#include <QThread>
#include "core/Types.h"
#include "win/XInputWrapper.h"

class GamepadPoller : public QObject {
    Q_OBJECT
public:
    explicit GamepadPoller(QObject* parent = nullptr);
    ~GamepadPoller();

    void start();
    void stop();

signals:
    void gamepadStateChanged(int controllerIndex, const GamepadState& state);
    void gamepadConnected(int controllerIndex);
    void gamepadDisconnected(int controllerIndex);

private:
    class PollThread : public QThread {
    public:
        PollThread(GamepadPoller* parent);
    protected:
        void run() override;
    private:
        GamepadPoller* m_parent;
    };

    void pollOnce();

    XInputWrapper m_xinput;
    PollThread m_thread;
    std::array<GamepadState, kMaxGamepads> m_prevState;
};
