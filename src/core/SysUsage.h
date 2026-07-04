#pragma once

#include <QObject>

// Samples total CPU usage and (approximate) GPU usage on Windows.
// GPU usage is best-effort via D3DKMT node engine time; may be unavailable
// on some drivers, in which case gpuUsage() returns -1.0.
class SysUsage : public QObject {
    Q_OBJECT
public:
    explicit SysUsage(QObject* parent = nullptr);
    ~SysUsage();

    // Returns total CPU usage in percent [0..100], or -1.0 on error / first call.
    // Each call computes the delta since the previous call.
    double cpuUsage();

    // Returns GPU usage in percent [0..100], or -1.0 if unavailable.
    // Each call computes the delta since the previous call.
    double gpuUsage();

private:
    // CPU via GetSystemTimes (idle/kernel/user FILETIME)
    bool    m_cpuFirst = true;
    quint64 m_cpuIdlePrev = 0;
    quint64 m_cpuKernelPrev = 0;
    quint64 m_cpuUserPrev = 0;

    // GPU via D3DKMTQueryStatistics (node running vs total engine ticks)
    bool    m_gpuReady = false;       // adapter discovered
    bool    m_gpuFirst = true;
    unsigned long m_adapterLuidLow = 0;
    long    m_adapterLuidHigh = 0;
    int     m_nodeCount = 0;
    quint64 m_gpuRunningPrev = 0;
    quint64 m_qpcPrev = 0;
    void*   m_pQS = nullptr;          // cached D3DKMTQueryStatistics function pointer

    bool discoverAdapter();          // finds primary adapter LUID + node count
};
