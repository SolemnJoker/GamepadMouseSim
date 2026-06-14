#pragma once

#include <QObject>
#include <QApplication>
#include "core/Config.h"
#include "core/ModeManager.h"
#include "core/ProcessDetector.h"
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
    ModeManager m_modeManager;
    ProcessDetector m_processDetector;
    GamepadPoller m_gamepadPoller;
    ComboKeyDetector m_comboKeyDetector;
    InputMapper m_inputMapper;
    SystemTray m_systemTray;
    OsdOverlay m_osdOverlay;
};
