#include "Application.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

Application::Application(QObject* parent)
    : QObject(parent)
    , m_config(this)
    , m_gamepadPoller(this)
    , m_systemTray(this)
    , m_osdOverlay(nullptr)
{
    for (int i = 0; i < kMaxGamepads; ++i) {
        m_comboDetectors[i] = new ComboKeyDetector(i, this);
        m_inputMappers[i] = new InputMapper(&m_config, i, this);
        m_modeManagers[i] = new ModeManager(&m_config, i, this);
    }
}

Application::~Application() {
    m_gamepadPoller.stop();
}

bool Application::initialize() {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.json";
    qDebug() << "Looking for config at:" << configPath;

    bool loaded = m_config.load(configPath);
    if (!loaded) {
        configPath = QCoreApplication::applicationDirPath() + "/config/default_config.json";
        qDebug() << "Fallback to:" << configPath;
        loaded = m_config.load(configPath);
    }

    if (!loaded) {
        qDebug() << "WARNING: No config file found, using defaults";
    } else {
        qDebug() << "Config loaded successfully";
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        m_inputMappers[i]->onConfigChanged();
    }

    connect(&m_gamepadPoller, &GamepadPoller::gamepadStateChanged,
            this, [this](int idx, const GamepadState& state) {
                if (idx < kMaxGamepads) {
                    m_comboDetectors[idx]->onGamepadState(idx, state);
                    m_inputMappers[idx]->onGamepadStateChanged(idx, state);
                }
            });

    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_comboDetectors[i], &ComboKeyDetector::comboTriggered,
                this, [this](int idx) {
                    if (idx < kMaxGamepads) {
                        m_modeManagers[idx]->manualSwitch();
                    }
                });
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_modeManagers[i], &ModeManager::modeChanged,
                this, [this](int idx, GamepadMode mode) {
                    if (idx < kMaxGamepads) {
                        m_inputMappers[idx]->onModeChanged(mode);
                        m_systemTray.onModeChanged(idx, mode);
                        m_osdOverlay.showModeChange(idx, mode);
                    }
                });
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_inputMappers[i], &InputMapper::showHelpRequested,
                this, [this, i]() {
                    m_osdOverlay.showHelp(m_inputMappers[i]->helpText());
                });
    }

    connect(&m_systemTray, &SystemTray::switchModeRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->manualSwitch();
                }
            });
    connect(&m_systemTray, &SystemTray::lockModeRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->toggleLock();
                }
            });
    connect(&m_systemTray, &SystemTray::pauseRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->togglePause();
                }
            });
    connect(&m_systemTray, &SystemTray::exitRequested,
            qApp, &QApplication::quit);

    connect(&m_config, &Config::configChanged, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->onConfigChanged();
            m_inputMappers[i]->onConfigChanged();
        }
    });

    m_gamepadPoller.start();
    for (int i = 0; i < kMaxGamepads; ++i) {
        m_modeManagers[i]->start();
    }
    m_systemTray.show();

    return true;
}
