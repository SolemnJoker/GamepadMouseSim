#include "ModeManager.h"
#include "ProcessDetector.h"
#include "Config.h"
#include <QDebug>

ModeManager::ModeManager(Config* config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &ModeManager::onPollTimer);
    connect(&m_lockoutTimer, &QTimer::timeout, this, [this]() {
        qDebug() << "Lockout expired, resuming auto detection";
        m_pollTimer.start(m_pollIntervalMs);
    });
}

void ModeManager::start() {
    loadConfig();
    qDebug() << "ModeManager started, initial mode:" << (m_mode == GamepadMode::Mouse ? "Mouse" : "Default");
    m_pollTimer.start(m_pollIntervalMs);
}

void ModeManager::manualSwitch() {
    if (m_locked) {
        qDebug() << "Manual switch blocked - mode is locked";
        return;
    }
    GamepadMode newMode = (m_mode == GamepadMode::Default) ? GamepadMode::Mouse : GamepadMode::Default;
    qDebug() << "Manual switch from" << (m_mode == GamepadMode::Mouse ? "Mouse" : "Default")
             << "to" << (newMode == GamepadMode::Mouse ? "Mouse" : "Default");
    setMode(newMode);
    m_pollTimer.stop();
    m_lockoutTimer.start(m_lockoutMs);
}

void ModeManager::toggleLock() {
    m_locked = !m_locked;
    qDebug() << "Mode lock:" << (m_locked ? "ON" : "OFF");
    if (m_locked) {
        m_pollTimer.stop();
        m_lockoutTimer.stop();
    } else {
        m_pollTimer.start(m_pollIntervalMs);
    }
}

void ModeManager::togglePause() {
    m_paused = !m_paused;
    qDebug() << "Passthrough pause:" << (m_paused ? "ON" : "OFF");
}

void ModeManager::onConfigChanged() {
    loadConfig();
}

void ModeManager::onPollTimer() {
    if (m_paused || m_locked) return;

    ProcessDetector detector;
    QStringList processList = m_config->value("monitoring.process_list").toStringList();
    bool found = detector.isTargetRunning(processList);

    if (found && m_mode != GamepadMode::Default) {
        qDebug() << "Target process found, switching to Default mode";
        setMode(GamepadMode::Default);
    } else if (!found && m_mode != GamepadMode::Mouse) {
        qDebug() << "No target process, switching to Mouse mode";
        setMode(GamepadMode::Mouse);
    }
}

void ModeManager::setMode(GamepadMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        qDebug() << "Mode changed to:" << (mode == GamepadMode::Mouse ? "Mouse" : "Default");
        emit modeChanged(mode);
    }
}

void ModeManager::loadConfig() {
    m_pollIntervalMs = m_config->value("monitoring.polling_interval_seconds", 2).toInt() * 1000;
    m_lockoutMs = m_config->value("monitoring.manual_switch_lockout_seconds", 30).toInt() * 1000;
    qDebug() << "Config loaded - poll interval:" << m_pollIntervalMs << "ms, lockout:" << m_lockoutMs << "ms";
}
