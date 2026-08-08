#pragma once

#include "Types.h"
#include <QObject>
#include <QTimer>
#include <array>

class Config;
class ModeManager;
class GameDetector;

// Periodically evaluates whether the user is playing a game and, subject to
// the auto-switch rules, switches Mouse-mode pads to Default mode.
//
// Rules (per-pad):
//   2.1  If a pad is currently in Default mode -> never auto-switch it
//        (we do not auto-switch back, per the chosen behavior).
//   2.2  If a pad is currently in Mouse mode and a game is detected,
//        auto-switch it to Default.
//   2.2b Boot defaults to Default mode (handled by ModeManager).
class AutoModeController : public QObject {
    Q_OBJECT
  public:
    explicit AutoModeController(Config* config, std::array<ModeManager*, kMaxGamepads> modeManagers,
                                QObject* parent = nullptr);
    ~AutoModeController();

    void start();
    void stop();

  public slots:
    void onConfigChanged();

  private slots:
    void onTimeout();

  private:
    void loadConfig();
    void applyInterval();

    Config* m_config;
    std::array<ModeManager*, kMaxGamepads> m_modeManagers;
    GameDetector* m_detector;
    QTimer m_timer;

    bool m_enabled = false;
    int m_intervalSec = 10;
};
