#include "GameDetector.h"
#include "Config.h"
#include "ProcessDetector.h"
#include "SysUsage.h"
#include <QDateTime>
#include <QDebug>
#include <windows.h>

GameDetector::GameDetector(Config* config, QObject* parent)
    : QObject(parent), m_config(config), m_sys(new SysUsage(this)),
      m_procDetector(new ProcessDetector(this)) {
    loadCachedConfig();
}

void GameDetector::loadCachedConfig() {
    m_processEnabled = m_config->value("auto_switch.detection.process_list_enabled", true).toBool();
    m_fullscreenEnabled =
        m_config->value("auto_switch.detection.fullscreen_enabled", false).toBool();
    m_cpuEnabled = m_config->value("auto_switch.detection.cpu_enabled", false).toBool();
    m_cpuThreshold = m_config->value("auto_switch.detection.cpu_threshold", 50).toDouble();
    m_cpuSustainedSec =
        m_config->value("auto_switch.detection.cpu_sustained_seconds", 30).toDouble();
    m_gpuEnabled = m_config->value("auto_switch.detection.gpu_enabled", false).toBool();
    m_gpuThreshold = m_config->value("auto_switch.detection.gpu_threshold", 50).toDouble();
    m_gpuSustainedSec =
        m_config->value("auto_switch.detection.gpu_sustained_seconds", 30).toDouble();

    // Parse process names from QVariantList
    m_processNames.clear();
    QVariant v = m_config->value("auto_switch.detection.process_names");
    if (v.canConvert<QVariantList>()) {
        for (const QVariant& item : v.toList()) {
            QString t = item.toString().trimmed();
            if (!t.isEmpty())
                m_processNames << t;
        }
    }
}

void GameDetector::reloadConfig() {
    m_cpuSustainedAccumMs = 0.0;
    m_gpuSustainedAccumMs = 0.0;
    m_lastSampleMs = QDateTime::currentMSecsSinceEpoch();
    loadCachedConfig();
}

bool GameDetector::isFullscreenForeground() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return false;

    // Exclude shell / taskbar windows.
    wchar_t cls[256] = {};
    if (GetClassNameW(hwnd, cls, 256) > 0) {
        QString name = QString::fromWCharArray(cls);
        if (name.compare("Shell_TrayWnd", Qt::CaseInsensitive) == 0)
            return false; // taskbar
        if (name.compare("Progman", Qt::CaseInsensitive) == 0)
            return false; // desktop
        if (name.compare("WorkerW", Qt::CaseInsensitive) == 0)
            return false; // desktop
    }

    // Must be visible & not iconified.
    if (!IsWindowVisible(hwnd))
        return false;
    if (IsIconic(hwnd))
        return false;

    RECT rc;
    if (!GetWindowRect(hwnd, &rc))
        return false;

    // Compare against the union of all monitors (a window spanning the whole
    // primary screen counts). Use the nearest monitor's work vs screen area.
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hmon, &mi))
        return false;

    LONG w = rc.right - rc.left;
    LONG h = rc.bottom - rc.top;
    LONG sw = mi.rcMonitor.right - mi.rcMonitor.left;
    LONG sh = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // Allow a small tolerance for border/offset rounding.
    const LONG tol = 8;
    if (w >= sw - tol && h >= sh - tol) {
        // additionally the window origin should cover the monitor origin
        if (rc.left <= mi.rcMonitor.left + tol && rc.top <= mi.rcMonitor.top + tol) {
            return true;
        }
    }
    return false;
}

GameDetector::Result GameDetector::detect(qint64 nowMs) {
    Result result;

    // --- 1. Process list (instant) ---
    if (m_processEnabled && !m_processNames.isEmpty()) {
        if (m_procDetector->isTargetRunning(m_processNames)) {
            result.isPlaying = true;
            result.reason = QStringLiteral("检测到游戏进程");
            m_cpuSustainedAccumMs = 0.0;
            m_gpuSustainedAccumMs = 0.0;
            return result;
        }
    }

    // --- 2. Foreground fullscreen (instant) ---
    if (m_fullscreenEnabled && isFullscreenForeground()) {
        result.isPlaying = true;
        result.reason = QStringLiteral("前台窗口全屏");
        m_cpuSustainedAccumMs = 0.0;
        m_gpuSustainedAccumMs = 0.0;
        return result;
    }

    // --- 3 & 4. CPU / GPU sustained usage ---
    double dtMs = 0.0;
    if (m_lastSampleMs > 0) {
        dtMs = static_cast<double>(nowMs - m_lastSampleMs);
        if (dtMs < 0)
            dtMs = 0;
        if (dtMs > 60000)
            dtMs = 60000;
    }
    m_lastSampleMs = nowMs;

    if (m_cpuEnabled) {
        double cpu = m_sys->cpuUsage();
        if (cpu >= 0) {
            if (cpu >= m_cpuThreshold) {
                m_cpuSustainedAccumMs += dtMs;
            } else {
                m_cpuSustainedAccumMs = 0.0;
            }
            double needMs = m_cpuSustainedSec * 1000.0;
            if (m_cpuSustainedAccumMs >= needMs) {
                result.isPlaying = true;
                result.reason = QStringLiteral("CPU 占用持续偏高");
                m_cpuSustainedAccumMs = 0.0;
                return result;
            }
        }
    } else {
        m_cpuSustainedAccumMs = 0.0;
    }

    if (m_gpuEnabled) {
        double gpu = m_sys->gpuUsage();
        if (gpu >= 0) {
            if (gpu >= m_gpuThreshold) {
                m_gpuSustainedAccumMs += dtMs;
            } else {
                m_gpuSustainedAccumMs = 0.0;
            }
            double needMs = m_gpuSustainedSec * 1000.0;
            if (m_gpuSustainedAccumMs >= needMs) {
                result.isPlaying = true;
                result.reason = QStringLiteral("GPU 占用持续偏高");
                m_gpuSustainedAccumMs = 0.0;
                return result;
            }
        }
    } else {
        m_gpuSustainedAccumMs = 0.0;
    }

    return result;
}
