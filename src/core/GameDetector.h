#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class Config;

// Evaluates a single "is the user playing a game?" decision based on the
// configured detection sources. Each source is OR-combined.
//
// Sources:
//   1. Game process list   (instant)
//   2. Foreground window is fullscreen (instant)
//   3. CPU usage sustained above threshold (stateful)
//   4. GPU usage sustained above threshold (stateful)
//
// CPU/GPU are tracked with sustained-seconds windows so that instantaneous
// spikes don't trigger a switch.
class GameDetector : public QObject {
    Q_OBJECT
public:
    struct Result {
        bool   isPlaying = false;
        QString reason;     // human-readable hit reason, empty if not playing
    };

    explicit GameDetector(Config* config, QObject* parent = nullptr);

    // Advance the sustained-usage timers and return the current decision.
    Result detect(qint64 nowMs);

    // Called on config change to reset thresholds/lists.
    void reloadConfig();

private:
    // Static helpers
    static bool isFullscreenForeground();

    Config* m_config;

    // CPU/GPU sustained-tracking state (seconds above threshold)
    double m_cpuSustainedAccumMs = 0.0;
    double m_gpuSustainedAccumMs = 0.0;
    qint64 m_lastSampleMs = 0;

    // Cached usage sampler
    class SysUsage* m_sys = nullptr;

    // Persistent process detector (avoids per-tick construction)
    class ProcessDetector* m_procDetector = nullptr;

    // Cached config values (reloaded on reloadConfig())
    bool        m_processEnabled = true;
    QStringList m_processNames;
    bool        m_fullscreenEnabled = false;
    bool        m_cpuEnabled = false;
    double      m_cpuThreshold = 50.0;
    double      m_cpuSustainedSec = 30.0;
    bool        m_gpuEnabled = false;
    double      m_gpuThreshold = 50.0;
    double      m_gpuSustainedSec = 30.0;

    void loadCachedConfig();
};
