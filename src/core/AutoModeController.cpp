#include "AutoModeController.h"
#include "Config.h"
#include "ModeManager.h"
#include "GameDetector.h"
#include "core/Types.h"
#include <QDateTime>
#include <QDebug>

AutoModeController::AutoModeController(Config* config,
                                       std::array<ModeManager*, kMaxGamepads> modeManagers,
                                       QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_modeManagers(modeManagers)
    , m_detector(new GameDetector(config, this))
{
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &AutoModeController::onTimeout);
    loadConfig();
}

AutoModeController::~AutoModeController() {
    stop();
}

void AutoModeController::loadConfig() {
    bool wasEnabled = m_enabled;
    m_enabled    = m_config->value("auto_switch.enabled", false).toBool();
    m_intervalSec = m_config->value("auto_switch.poll_interval_seconds", 10).toInt();
    if (m_intervalSec < 1) m_intervalSec = 1;

    m_detector->reloadConfig();
    applyInterval();

    if (m_enabled && !wasEnabled) {
        qDebug() << "AutoModeController enabled (interval" << m_intervalSec << "s)";
    } else if (!m_enabled && wasEnabled) {
        qDebug() << "AutoModeController disabled";
    }
}

void AutoModeController::applyInterval() {
    if (m_timer.isActive()) {
        m_timer.start(m_intervalSec * kMsPerSecond);
    }
}

void AutoModeController::start() {
    if (!m_enabled) {
        qDebug() << "AutoModeController: disabled, not starting timer";
        return;
    }
    m_timer.start(m_intervalSec * kMsPerSecond);
    qDebug() << "AutoModeController started";
}

void AutoModeController::stop() {
    m_timer.stop();
}

void AutoModeController::onConfigChanged() {
    loadConfig();
    // start/stop the timer based on the new enabled flag
    if (m_enabled) {
        if (!m_timer.isActive()) {
            m_timer.start(m_intervalSec * kMsPerSecond);
        }
    } else {
        m_timer.stop();
    }
}

void AutoModeController::onTimeout() {
    if (!m_enabled) return;

    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    GameDetector::Result r = m_detector->detect(nowMs);

    if (!r.isPlaying) {
        return;
    }

    qDebug() << "AutoModeController: game detected -" << r.reason;

    // 2.1 / 2.2 per-pad rule.
    for (int i = 0; i < kMaxGamepads; ++i) {
        if (!m_modeManagers[i]) continue;
        GamepadMode cur = m_modeManagers[i]->currentMode();
        if (cur == GamepadMode::Mouse) {
            // 2.2: Mouse -> Default when playing.
            m_modeManagers[i]->autoSwitchToDefault(r.reason);
        }
        // 2.1: Default mode is left untouched (no auto-switch back).
    }
}
