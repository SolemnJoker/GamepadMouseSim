#include "Application.h"
#include "ui/SettingsDialog.h"
#include "core/AutoStart.h"
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
    m_autoController = new AutoModeController(&m_config, m_modeManagers, this);
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
                this, [this]() {
                    m_osdOverlay.showHelp();
                });
    }

    connect(&m_systemTray, &SystemTray::switchModeRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->manualSwitch();
                }
            });
    connect(&m_systemTray, &SystemTray::settingsRequested,
            this, [this]() { showSettings(); });
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
        m_autoController->onConfigChanged();
    });

    // Apply the configured boot autostart state on every launch.
    if (m_config.value("autostart", false).toBool()) {
        if (!AutoStart::isEnabled()) {
            AutoStart::setEnabled(true);
        }
    } else {
        if (AutoStart::isEnabled()) {
            AutoStart::setEnabled(false);
        }
    }

    m_gamepadPoller.start();
    for (int i = 0; i < kMaxGamepads; ++i) {
        m_modeManagers[i]->start();
    }
    m_autoController->start();
    m_systemTray.show();

    return true;
}

void Application::showSettings() {
    if (m_settingsDialog) {
        m_settingsDialog->raise();
        m_settingsDialog->activateWindow();
        return;
    }
    m_settingsDialog = new SettingsDialog(&m_config, nullptr);
    m_settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_settingsDialog, &QDialog::finished, this, [this]() {
        m_settingsDialog = nullptr;
    });
    m_settingsDialog->show();
}
