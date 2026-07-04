#include "AutoStart.h"
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

const QString AutoStart::kRunKey   = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const QString AutoStart::kValueName = "GamepadMouseSim";

QString AutoStart::executablePath() {
    // QCoreApplication::applicationFilePath() returns the .exe path with native separators.
    QString path = QCoreApplication::applicationFilePath();
    return path;
}

bool AutoStart::isEnabled() {
    QSettings settings(kRunKey, QSettings::NativeFormat);
    return settings.contains(kValueName);
}

void AutoStart::setEnabled(bool enabled) {
    QSettings settings(kRunKey, QSettings::NativeFormat);
    if (enabled) {
        // Quote the path so spaces in the directory don't break execution.
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
