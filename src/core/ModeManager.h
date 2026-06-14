#pragma once

#include <QObject>
#include <QTimer>
#include "Types.h"

class Config;

class ModeManager : public QObject {
    Q_OBJECT
public:
    explicit ModeManager(Config* config, QObject* parent = nullptr);

    void start();
    GamepadMode currentMode() const { return m_mode; }
    bool isLocked() const { return m_locked; }
    bool isPaused() const { return m_paused; }

public slots:
    void manualSwitch();
    void toggleLock();
    void togglePause();
    void onConfigChanged();

signals:
    void modeChanged(GamepadMode mode);

private slots:
    void onPollTimer();

private:
    void setMode(GamepadMode mode);
    void loadConfig();

    Config* m_config;
    GamepadMode m_mode = GamepadMode::Mouse;
    bool m_locked = false;
    bool m_paused = false;
    QTimer m_pollTimer;
    QTimer m_lockoutTimer;
    int m_pollIntervalMs = kDefaultPollingIntervalMs;
    int m_lockoutMs = kDefaultManualLockoutMs;
};
