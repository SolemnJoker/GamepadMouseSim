#include "Application.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

Application::Application(QObject* parent)
    : QObject(parent)
    , m_config(this)
    , m_modeManager(&m_config, this)
    , m_processDetector(this)
    , m_gamepadPoller(this)
    , m_comboKeyDetector(this)
    , m_inputMapper(&m_config, this)
    , m_systemTray(this)
    , m_osdOverlay(nullptr)
{
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

    m_inputMapper.onConfigChanged();

    connect(&m_gamepadPoller, &GamepadPoller::gamepadStateChanged,
            &m_comboKeyDetector, &ComboKeyDetector::onGamepadState);
    connect(&m_gamepadPoller, &GamepadPoller::gamepadStateChanged,
            &m_inputMapper, &InputMapper::onGamepadStateChanged);

    connect(&m_comboKeyDetector, &ComboKeyDetector::comboTriggered,
            &m_modeManager, &ModeManager::manualSwitch);

    connect(&m_modeManager, &ModeManager::modeChanged,
            &m_inputMapper, &InputMapper::onModeChanged);
    connect(&m_modeManager, &ModeManager::modeChanged,
            &m_systemTray, &SystemTray::onModeChanged);
    connect(&m_modeManager, &ModeManager::modeChanged,
            &m_osdOverlay, &OsdOverlay::showModeChange);

    connect(&m_systemTray, &SystemTray::switchModeRequested,
            &m_modeManager, &ModeManager::manualSwitch);
    connect(&m_systemTray, &SystemTray::lockModeRequested,
            &m_modeManager, &ModeManager::toggleLock);
    connect(&m_systemTray, &SystemTray::pauseRequested,
            &m_modeManager, &ModeManager::togglePause);
    connect(&m_systemTray, &SystemTray::exitRequested,
            qApp, &QApplication::quit);

    connect(&m_config, &Config::configChanged, &m_modeManager, &ModeManager::onConfigChanged);
    connect(&m_config, &Config::configChanged, &m_inputMapper, &InputMapper::onConfigChanged);

    m_gamepadPoller.start();
    m_modeManager.start();
    m_systemTray.show();

    return true;
}
