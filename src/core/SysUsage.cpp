#include "SysUsage.h"
#include <windows.h>
#include <dxgi.h>
#include <QDebug>

// IID for IDXGIFactory1 (defined by MinGW-w64 in dxgi.h; define if missing).
#ifndef IID_IDXGIFactory1
DEFINE_GUID(IID_IDXGIFactory1,
    0x770aae78, 0x972d, 0x4e11, 0xa2, 0xe6, 0x08, 0xd4, 0xa4, 0x78, 0x03, 0xef);
#endif

// ---- D3DKMT forward decls (MinGW may not ship d3dkmthk.h) ----
extern "C" {
typedef NTSTATUS (WINAPI *PFN_D3DKMTQueryStatistics)(void*);
}

// =====================================================================
// CPU usage via GetSystemTimes
// =====================================================================
SysUsage::SysUsage(QObject* parent)
    : QObject(parent)
{
    m_gpuReady = discoverAdapter();
}

SysUsage::~SysUsage() {
}

double SysUsage::cpuUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return -1.0;
    }

    quint64 idle   = (static_cast<quint64>(idleTime.dwHighDateTime) << 32)   | idleTime.dwLowDateTime;
    quint64 kernel = (static_cast<quint64>(kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
    quint64 user   = (static_cast<quint64>(userTime.dwHighDateTime) << 32)   | userTime.dwLowDateTime;

    if (m_cpuFirst) {
        m_cpuFirst = false;
        m_cpuIdlePrev = idle;
        m_cpuKernelPrev = kernel;
        m_cpuUserPrev = user;
        return -1.0;  // need two samples
    }

    quint64 idleDelta   = idle   - m_cpuIdlePrev;
    quint64 kernelDelta = kernel - m_cpuKernelPrev;
    quint64 userDelta   = user   - m_cpuUserPrev;

    // kernel includes idle, so total busy = (kernel + user) - idle
    quint64 totalDelta = kernelDelta + userDelta;
    double usage = 0.0;
    if (totalDelta > 0) {
        usage = (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta)) * 100.0;
        if (usage < 0.0) usage = 0.0;
        if (usage > 100.0) usage = 100.0;
    }

    m_cpuIdlePrev = idle;
    m_cpuKernelPrev = kernel;
    m_cpuUserPrev = user;
    return usage;
}

// =====================================================================
// GPU usage via D3DKMT node running time.
//
// D3DKMTQueryStatistics for QueryNodeType expects a buffer with this
// known layout (from WDK d3dkmthk.h, 64-bit):
//   offset  0: ULONG  Type             = 5 (D3DKMT_QUERYSTATISTICS_NODE)
//   offset  4: LUID   AdapterLuid      (8 bytes)
//   offset 12: ULONG  hProcess         = 0
//   offset 16: ULONG  NodeId           = <node index>
//   offset 20: ... (output written by kernel)
//
// The kernel writes a D3DKMT_QUERYSTATISTICS_NODE_RESULT starting after
// the input fields.  The first member (RunningTime, ULONGLONG) sits at
// an architecture-dependent offset.  On x64 it starts after padding at
// offset 24.  We use a raw byte buffer and memcpy to avoid depending on
// WDK headers or ABI-sensitive struct layouts.
// =====================================================================

#if defined(_WIN64)
static constexpr int kNodeIdOffset = 16;        // ULONG at offset 16
static constexpr int kRunningTimeOffset = 24;    // ULONGLONG at offset 24
#else
static constexpr int kNodeIdOffset = 16;
static constexpr int kRunningTimeOffset = 20;
#endif

bool SysUsage::discoverAdapter() {
    // Enumerate DXGI adapters to find the first hardware adapter.
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1,
                                  reinterpret_cast<void**>(&factory)))) {
        return false;
    }

    IDXGIAdapter1* adapter = nullptr;
    LUID primaryLuid = {};
    bool found = false;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(adapter->GetDesc1(&desc))) {
            // Skip Microsoft Basic Render Driver (WARP).
            if (!(desc.VendorId == 0x1414 && desc.DeviceId == 0x8C)) {
                primaryLuid = desc.AdapterLuid;
                found = true;
                adapter->Release();
                break;
            }
        }
        adapter->Release();
    }
    factory->Release();
    if (!found) return false;

    m_adapterLuidLow  = primaryLuid.LowPart;
    m_adapterLuidHigh = primaryLuid.HighPart;

    // Get the D3DKMTQueryStatistics function pointer.
    HMODULE gdi32 = GetModuleHandleW(L"gdi32.dll");
    if (!gdi32) return false;
    m_pQS = reinterpret_cast<void*>(GetProcAddress(gdi32, "D3DKMTQueryStatistics"));
    if (!m_pQS) return false;
    auto pQS = reinterpret_cast<PFN_D3DKMTQueryStatistics>(m_pQS);

    // Probe node count by querying until failure.
    // Use a raw buffer large enough for kernel output.
    static constexpr size_t kBufSize = 4096;
    UCHAR buf[kBufSize];
    int nodes = 0;
    for (int n = 0; n < 64; ++n) {
        ZeroMemory(buf, kBufSize);

        // Fill input fields.
        *reinterpret_cast<ULONG*>(buf + 0)  = 5;  // D3DKMT_QUERYSTATISTICS_NODE
        *reinterpret_cast<LUID*>(buf + 4)   = primaryLuid;
        *reinterpret_cast<ULONG*>(buf + 12)  = 0;  // hProcess
        *reinterpret_cast<ULONG*>(buf + kNodeIdOffset) = static_cast<ULONG>(n);

        NTSTATUS st = pQS(buf);
        if (st != 0) break;
        nodes = n + 1;
    }

    if (nodes <= 0) return false;
    m_nodeCount = nodes;
    qDebug() << "SysUsage: GPU adapter found, node count =" << nodes;
    return true;
}

double SysUsage::gpuUsage() {
    if (!m_gpuReady || m_nodeCount <= 0 || !m_pQS) return -1.0;

    auto pQS = reinterpret_cast<PFN_D3DKMTQueryStatistics>(m_pQS);

    LUID luid;
    luid.LowPart  = m_adapterLuidLow;
    luid.HighPart = m_adapterLuidHigh;

    static constexpr size_t kBufSize = 4096;
    UCHAR buf[kBufSize];
    quint64 runningSum = 0;

    for (int n = 0; n < m_nodeCount; ++n) {
        ZeroMemory(buf, kBufSize);

        *reinterpret_cast<ULONG*>(buf + 0)  = 5;
        *reinterpret_cast<LUID*>(buf + 4)   = luid;
        *reinterpret_cast<ULONG*>(buf + 12)  = 0;
        *reinterpret_cast<ULONG*>(buf + kNodeIdOffset) = static_cast<ULONG>(n);

        NTSTATUS st = pQS(buf);
        if (st != 0) break;

        ULONGLONG runningTime = 0;
        memcpy(&runningTime, buf + kRunningTimeOffset, sizeof(ULONGLONG));
        runningSum += runningTime;
    }

    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    quint64 qpcNow = static_cast<quint64>(qpc.QuadPart);

    if (m_gpuFirst) {
        m_gpuFirst = false;
        m_gpuRunningPrev = runningSum;
        m_qpcPrev = qpcNow;
        return -1.0;
    }

    quint64 qpcDelta = qpcNow - m_qpcPrev;
    quint64 runDelta = runningSum - m_gpuRunningPrev;

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    double usage = -1.0;
    if (qpcDelta > 0 && freq.QuadPart > 0) {
        // RunningTime is in 100ns units; convert QPC to 100ns.
        double wall100ns = static_cast<double>(qpcDelta) / static_cast<double>(freq.QuadPart) * 1e7;
        // Normalize across nodes.
        double busy100ns = static_cast<double>(runDelta) / static_cast<double>(m_nodeCount);
        usage = busy100ns / wall100ns * 100.0;
        if (usage < 0.0) usage = 0.0;
        if (usage > 100.0) usage = 100.0;
    }

    m_gpuRunningPrev = runningSum;
    m_qpcPrev = qpcNow;
    return usage;
}
