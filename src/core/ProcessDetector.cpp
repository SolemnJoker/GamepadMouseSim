#include "ProcessDetector.h"
#include <windows.h>
#include <tlhelp32.h>

ProcessDetector::ProcessDetector(QObject* parent)
    : QObject(parent)
{
}

bool ProcessDetector::isTargetRunning(const QStringList& processNames) {
    if (processNames.isEmpty()) return false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    bool found = false;
    if (Process32FirstW(snapshot, &pe)) {
        do {
            for (const QString& name : processNames) {
                if (_wcsicmp(pe.szExeFile, name.toStdWString().c_str()) == 0) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return found;
}
