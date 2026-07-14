#pragma once

#include "core/AutoModeController.h"
#include "core/Config.h"
#include "core/ModeManager.h"
#include "gamepad/ComboKeyDetector.h"
#include "gamepad/GamepadPoller.h"
#include "input/InputMapper.h"
#include "ui/OsdOverlay.h"
#include "ui/SystemTray.h"
#include <QApplication>
#include <QObject>
#include <array>

class QDialog;

class Application : public QObject {
    Q_OBJECT
  public:
    explicit Application(QObject* parent = nullptr);
    ~Application();

    bool initialize();

  private slots:
    void showSettings();

  private:
    Config m_config;
    GamepadPoller m_gamepadPoller;
    SystemTray m_systemTray;
    OsdOverlay m_osdOverlay;
    AutoModeController* m_autoController = nullptr;

    std::array<ComboKeyDetector*, kMaxGamepads> m_comboDetectors;
    std::array<InputMapper*, kMaxGamepads> m_inputMappers;
    std::array<ModeManager*, kMaxGamepads> m_modeManagers;
    QDialog* m_settingsDialog = nullptr;
    bool m_lastAutostart = false;
};
