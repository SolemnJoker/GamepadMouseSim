#pragma once

#include <QString>

// Manages boot auto-start via HKCU\...\Run registry key (no admin needed).
// The single public entry point is applyToRegistry(); the surrounding
// config/subsystem code is responsible for deciding *when* to call it.
class AutoStart {
public:
    static const QString kRunKey;       // "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
    static const QString kValueName;    // "GamepadMouseSim"

    static void applyToRegistry(bool enabled);
    static QString executablePath();
};
