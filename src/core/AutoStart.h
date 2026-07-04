#pragma once

#include <QString>

// Manages boot auto-start via HKCU\...\Run registry key (no admin needed).
class AutoStart {
public:
    static const QString kRunKey;       // "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
    static const QString kValueName;    // "GamepadMouseSim"

    static bool isEnabled();
    // Sets/clears the auto-start entry. The exe path is the running process path.
    static void setEnabled(bool enabled);
    // Returns the absolute path of the running executable.
    static QString executablePath();
};
