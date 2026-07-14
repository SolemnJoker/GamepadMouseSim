#pragma once

#include "Types.h"
#include <QObject>
#include <QTimer>

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
    bool isAutoSwitched() const { return m_autoSwitched; }

  public slots:
    void manualSwitch();
    void toggleLock();
    void togglePause();
    void onConfigChanged();
    // Auto-switch (used by AutoModeController): goes Mouse -> Default.
    // Does NOT respect the manual lockout, since the user explicitly enabled
    // auto-switching in config. The reason string is logged only.
    void autoSwitchToDefault(const QString& reason = QString());

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
    bool m_autoSwitched = false; // true if the current Default state was reached by auto-switch
    QTimer m_lockoutTimer;
    int m_lockoutMs = kDefaultManualLockoutMs;
};
