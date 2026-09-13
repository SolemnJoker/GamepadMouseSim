#include "AutoStart.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
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

QString AutoStart::normalizePath(const QString& quotedPath) {
    QString p = quotedPath;
    if (p.startsWith('"') && p.endsWith('"') && p.size() >= 2)
        p = p.mid(1, p.size() - 2);
    return QDir::fromNativeSeparators(p).toLower();
}

bool AutoStart::shouldRemoveOnDisable(const QString& existingValue) {
    const QString normalized = normalizePath(existingValue);
    if (normalized.isEmpty())
        return false;
    // Ours to remove: it points at this executable, or at a path that no
    // longer exists (user moved/renamed/deleted the folder since enabling).
    if (normalized == normalizePath(executablePath()))
        return true;
    return !QFileInfo::exists(QDir::toNativeSeparators(normalized));
}

bool AutoStart::runValuePointsToCurrentExe() {
    QString existing;
    if (IRegistry* r = registry())
        existing = r->contains(kValueName) ? r->value(kValueName) : QString();
    else {
        QSettings settings(kRunKey, QSettings::NativeFormat);
        existing = settings.value(kValueName).toString();
    }
    return !existing.isEmpty() && normalizePath(existing) == normalizePath(executablePath());
}

void AutoStart::applyToRegistry(bool enabled) {
    if (IRegistry* r = registry()) {
        // Test path: drive through the injected fake.
        if (enabled) {
            r->setValue(kValueName, "\"" + executablePath() + "\"");
            qDebug() << "Autostart enabled:" << executablePath();
        } else {
            const QString existing = r->contains(kValueName) ? r->value(kValueName) : QString();
            if (shouldRemoveOnDisable(existing)) {
                r->remove(kValueName);
                qDebug() << "Autostart disabled (own or dead registration removed)";
            } else {
                qDebug() << "Autostart registration left untouched (other live copy):" << existing;
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
        const QString existing =
            settings.contains(kValueName) ? settings.value(kValueName).toString() : QString();
        if (shouldRemoveOnDisable(existing)) {
            settings.remove(kValueName);
            qDebug() << "Autostart disabled (own or dead registration removed)";
        } else {
            qDebug() << "Autostart registration left untouched (other live copy):" << existing;
        }
    }
    settings.sync();
}
