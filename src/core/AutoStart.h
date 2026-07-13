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

// Abstract registry seam so AutoStart can be unit-tested without touching HKCU.
// Production callers don't construct this; the inline QSettings path in
// applyToRegistry() is used. Tests inject a FakeRegistry via
// setRegistryForTesting() and call applyToRegistry() exactly as production code
// would.
class IRegistry {
public:
    virtual ~IRegistry() = default;
    virtual bool contains(const QString& valueName) const = 0;
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