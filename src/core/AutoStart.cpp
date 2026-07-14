#include "AutoStart.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>

namespace {
// Process-wide injection seam for AutoStart::applyToRegistry().
// nullptr in production; tests call setRegistryForTesting() to install a fake.
IRegistry* g_testRegistry = nullptr;
} // namespace

IRegistry* registry() {
    return g_testRegistry;
}

void setRegistryForTesting(IRegistry* fake) {
    g_testRegistry = fake;
}

const QString AutoStart::kRunKey =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const QString AutoStart::kValueName = "GamepadMouseSim";

QString AutoStart::executablePath() {
    // QCoreApplication::applicationFilePath() returns the .exe path with native separators.
    QString path = QCoreApplication::applicationFilePath();
    return path;
}

void AutoStart::applyToRegistry(bool enabled) {
    if (IRegistry* r = registry()) {
        // Test path: drive through the injected fake.
        if (enabled) {
            r->setValue(kValueName, "\"" + executablePath() + "\"");
            qDebug() << "Autostart enabled:" << executablePath();
        } else {
            if (r->contains(kValueName)) {
                r->remove(kValueName);
                qDebug() << "Autostart disabled";
            }
        }
        r->sync();
        return;
    }

    // Production path: real HKCU writes.
    QSettings settings(kRunKey, QSettings::NativeFormat);
    if (enabled) {
        QString exe = executablePath();
        settings.setValue(kValueName, "\"" + exe + "\"");
        qDebug() << "Autostart enabled:" << exe;
    } else {
        if (settings.contains(kValueName)) {
            settings.remove(kValueName);
            qDebug() << "Autostart disabled";
        }
    }
    settings.sync();
}