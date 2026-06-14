#pragma once

#include <QObject>
#include <QTimer>
#include "Types.h"

class Config;

class ModeManager : public QObject {
    Q_OBJECT
public:
    explicit ModeManager(Config* config, int controllerIndex, QObject* parent = nullptr);

    void start();
    GamepadMode currentMode() const { return m_mode; }
    bool isLocked() const { return m_locked; }
    bool isPaused() const { return m_paused; }
    int controllerIndex() const { return m_controllerIndex; }

public slots:
    void manualSwitch();
    void toggleLock();
    void togglePause();
    void onConfigChanged();

signals:
    void modeChanged(int controllerIndex, GamepadMode mode);

private:
    void setMode(GamepadMode mode);
    void loadConfig();

    Config* m_config;
    int m_controllerIndex;
    GamepadMode m_mode = GamepadMode::Mouse;
    bool m_locked = false;
    bool m_paused = false;
    QTimer m_lockoutTimer;
    int m_lockoutMs = kDefaultManualLockoutMs;
};
