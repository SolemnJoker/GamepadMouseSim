#pragma once

#include <QString>

// Manages boot auto-start via HKCU\...\Run registry key (no admin needed).
// The single public entry point is applyToRegistry(); the surrounding
// config/subsystem code is responsible for deciding *when* to call it.
//
// Disable semantics (portable-install self-healing, design A/B of the
// autostart work): disabling removes the Run value only when it is ours to
// touch — either it points at *this* executable, or it is a dead path left
// behind after the user moved/renamed the folder. A value pointing at some
// other live copy (a second installation on the same machine) is left alone.
class AutoStart {
  public:
    static const QString kRunKey;    // "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
    static const QString kValueName; // "GamepadMouseSim"

    static void applyToRegistry(bool enabled);
    static QString executablePath();

    // True when the HKCU Run value currently points at this executable.
    // Used at startup to adopt an externally-written registration (e.g. the
    // installer's optional auto-start task) into the config as authoritative.
    static bool runValuePointsToCurrentExe();

    // Normalization helpers shared by the checks (exposed for tests).
    static QString normalizePath(const QString& quotedPath); // strips quotes, / -> \, case-folded
    static bool shouldRemoveOnDisable(const QString& existingValue);
};

// Abstract registry seam so AutoStart can be unit-tested without touching HKCU.
// Production callers don't construct this; the inline QSettings path in
// applyToRegistry() is used. Tests inject a FakeRegistry via
// setRegistryForTesting() and call applyToRegistry() exactly as production code
// would.
class IRegistry {
  public:
    virtual ~IRegistry() = default;
    virtual bool contains(const QString& valueName) const = 0;
    virtual QString value(const QString& valueName) const = 0;
    virtual void setValue(const QString& valueName, const QString& value) = 0;
    virtual void remove(const QString& valueName) = 0;
    virtual void sync() = 0;
};

// Process-wide registry accessor. Returns nullptr when no fake has been
// injected (production code uses the inline QSettings path in
// AutoStart::applyToRegistry). Tests call setRegistryForTesting() to swap
// in a fake.
IRegistry* registry();
void setRegistryForTesting(IRegistry* fake);
