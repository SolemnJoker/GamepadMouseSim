#include "ModeManager.h"
#include "Config.h"
#include "core/Types.h"
#include <QDebug>

ModeManager::ModeManager(Config* config, int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_controllerIndex(controllerIndex)
{
    connect(&m_lockoutTimer, &QTimer::timeout, this, [this]() {
        qDebug() << "Lockout expired for Pad" << m_controllerIndex;
        m_lockoutTimer.stop();
    });
}

void ModeManager::start() {
    loadConfig();
    qDebug() << "ModeManager Pad" << m_controllerIndex << "started, mode: Mouse";
}

void ModeManager::manualSwitch() {
    if (m_locked) {
        qDebug() << "Manual switch blocked for Pad" << m_controllerIndex;
        return;
    }
    GamepadMode newMode = (m_mode == GamepadMode::Default) ? GamepadMode::Mouse : GamepadMode::Default;
    qDebug() << "Pad" << m_controllerIndex << "manual switch from"
             << (m_mode == GamepadMode::Mouse ? "Mouse" : "Default")
             << "to" << (newMode == GamepadMode::Mouse ? "Mouse" : "Default");
    // A manual switch clears the auto-switch origin flag, so the user stays in
    // control of the resulting mode.
    m_autoSwitched = false;
    setMode(newMode);
    m_lockoutTimer.start(m_lockoutMs);
}

void ModeManager::autoSwitchToDefault(const QString& reason) {
    if (m_mode == GamepadMode::Default) {
        // Already in default mode; nothing to do.
        return;
    }
    qDebug() << "Pad" << m_controllerIndex << "auto switch Mouse -> Default"
             << (reason.isEmpty() ? QString() : ("(" + reason + ")"));
    m_autoSwitched = true;
    setMode(GamepadMode::Default);
    // No lockout: auto-switch is a deliberate config-driven action, and the
    // user can still manually switch back afterwards.
}

void ModeManager::toggleLock() {
    m_locked = !m_locked;
    qDebug() << "Pad" << m_controllerIndex << "mode lock:" << (m_locked ? "ON" : "OFF");
    if (m_locked) {
        m_lockoutTimer.stop();
    }
}

void ModeManager::togglePause() {
    m_paused = !m_paused;
    qDebug() << "Pad" << m_controllerIndex << "pause:" << (m_paused ? "ON" : "OFF");
}

void ModeManager::onConfigChanged() {
    loadConfig();
}

void ModeManager::setMode(GamepadMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        qDebug() << "Pad" << m_controllerIndex << "mode changed to:" << (mode == GamepadMode::Mouse ? "Mouse" : "Default");
        emit modeChanged(m_controllerIndex, mode);
    }
}

void ModeManager::loadConfig() {
    m_lockoutMs = m_config->value("monitoring.manual_switch_lockout_seconds", 3).toInt() * kMsPerSecond;
    qDebug() << "Pad" << m_controllerIndex << "config loaded - lockout:" << m_lockoutMs << "ms";
}
