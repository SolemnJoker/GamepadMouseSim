#include "Application.h"
#include "core/AutoStart.h"
#include "input/KeyboardController.h"
#include "ui/KeyboardOverlay.h"
#include "ui/SettingsDialog.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>

Application::Application(QObject* parent)
    : QObject(parent), m_config(this), m_gamepadPoller(this), m_systemTray(this),
      m_osdOverlay(&m_config) {
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
        qWarning() << "No config file found, using defaults";
    } else {
        qDebug() << "Config loaded successfully";
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        m_inputMappers[i]->onConfigChanged();
    }

    // 虚拟键盘(design.md D3/D6):overlay 是无主顶层窗口(不抢焦点),
    // 控制器持有导航逻辑,打开期间的原始手柄输入在 gamepadStateChanged
    // 分发处旁路 InputMapper。
    m_keyboardOverlay = new KeyboardOverlay(nullptr);
    m_keyboardController =
        new KeyboardController(&m_config, m_keyInjector, m_keyboardOverlay, this);
    m_keyboardOverlay->setNavController(&m_keyboardController->nav());

    connect(&m_gamepadPoller, &GamepadPoller::gamepadStateChanged, this,
            [this](int idx, const GamepadState& state) {
                if (idx < kMaxGamepads) {
                    m_comboDetectors[idx]->onGamepadState(idx, state);
                    // 键盘打开时原始输入被 KeyboardController 消费(导航 +
                    // 关闭检测),不再进入 InputMapper(design.md D6 旁路);
                    // 模式切换组合键由上面的 ComboKeyDetector 承担,不受影响。
                    if (!m_keyboardController->onGamepadState(idx, state))
                        m_inputMappers[idx]->onGamepadStateChanged(idx, state);
                }
            });

    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_comboDetectors[i], &ComboKeyDetector::comboTriggered, this, [this](int idx) {
            if (idx < kMaxGamepads) {
                m_modeManagers[idx]->manualSwitch();
            }
        });
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_modeManagers[i], &ModeManager::modeChanged, this,
                [this](int idx, GamepadMode mode) {
                    if (idx < kMaxGamepads) {
                        m_inputMappers[idx]->onModeChanged(mode);
                        m_systemTray.onModeChanged(idx, mode);
                        m_osdOverlay.showModeChange(idx, mode);
                        m_keyboardController->onModeChanged(idx, mode);
                    }
                });
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_inputMappers[i], &InputMapper::showHelpRequested, this,
                [this]() { m_osdOverlay.showHelp(); });
        connect(m_inputMappers[i], &InputMapper::showKeyboardRequested, this,
                [this]() { m_keyboardController->openOverlay(); });
    }

    connect(&m_systemTray, &SystemTray::switchModeRequested, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->manualSwitch();
        }
    });
    connect(&m_systemTray, &SystemTray::settingsRequested, this, [this]() { showSettings(); });
    connect(&m_systemTray, &SystemTray::profileSwitchRequested, this,
            [this](const QString& name) { m_config.setActiveProfile(name); });
    connect(&m_systemTray, &SystemTray::restoreDefaultsRequested, this, [this]() {
        if (QMessageBox::question(nullptr, QStringLiteral("恢复默认配置"),
                                  QStringLiteral("将丢弃全部自定义配置(含所有操作方案),"
                                                 "恢复为出厂默认。确定继续?")) != QMessageBox::Yes)
            return;
        m_config.restoreFactoryDefaults();
        m_systemTray.rebuildProfileMenu(m_config.profileOrder(), m_config.activeProfileName());
    });
    connect(&m_systemTray, &SystemTray::lockModeRequested, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->toggleLock();
        }
    });
    connect(&m_systemTray, &SystemTray::pauseRequested, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->togglePause();
        }
    });
    connect(&m_systemTray, &SystemTray::exitRequested, qApp, &QApplication::quit);

    connect(&m_config, &Config::configChanged, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->onConfigChanged();
            m_inputMappers[i]->onConfigChanged();
        }
        m_autoController->onConfigChanged();
        m_keyboardController->onConfigChanged();
        m_systemTray.rebuildProfileMenu(m_config.profileOrder(), m_config.activeProfileName());

        // Autostart is the only key this file owns; sync the registry
        // whenever it changes. Skip if the value hasn't changed to avoid
        // redundant registry writes on unrelated config edits.
        const bool wantAutostart = m_config.value("autostart", false).toBool();
        if (wantAutostart != m_lastAutostart) {
            m_lastAutostart = wantAutostart;
            AutoStart::applyToRegistry(wantAutostart);
        }
    });

    // Apply the configured boot autostart state on every launch. A Run value
    // written by an external party (installer's optional auto-start task, or
    // a user editing the registry) that points at this exe is adopted into
    // the config once, so config and registry stay authoritative in one
    // place. Stale/dead-path values are healed by the disable branch below.
    bool wantAutostart = m_config.value("autostart", false).toBool();
    if (!wantAutostart && AutoStart::runValuePointsToCurrentExe()) {
        wantAutostart = true;
        m_config.setValue("autostart", true);
    }
    m_lastAutostart = wantAutostart;
    AutoStart::applyToRegistry(wantAutostart);

    m_gamepadPoller.start();
    m_systemTray.rebuildProfileMenu(m_config.profileOrder(), m_config.activeProfileName());
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
    connect(m_settingsDialog, &QDialog::finished, this, [this]() { m_settingsDialog = nullptr; });
    m_settingsDialog->show();
}
