#pragma once

#include <QObject>
#include <QApplication>
#include <array>
#include "core/Config.h"
#include "core/ModeManager.h"
#include "gamepad/GamepadPoller.h"
#include "gamepad/ComboKeyDetector.h"
#include "input/InputMapper.h"
#include "ui/SystemTray.h"
#include "ui/OsdOverlay.h"

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* parent = nullptr);
    ~Application();

    bool initialize();

private:
    Config m_config;
    GamepadPoller m_gamepadPoller;
    SystemTray m_systemTray;
    OsdOverlay m_osdOverlay;

    std::array<ComboKeyDetector*, kMaxGamepads> m_comboDetectors;
    std::array<InputMapper*, kMaxGamepads> m_inputMappers;
    std::array<ModeManager*, kMaxGamepads> m_modeManagers;
};
