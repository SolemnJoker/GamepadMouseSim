#pragma once

#include <QObject>
#include <windows.h>
#include <xinput.h>

// Test seam for injecting fake gamepad input.
class IXInput {
public:
    virtual ~IXInput() = default;
    virtual bool getState(DWORD userIndex, XINPUT_STATE* state) = 0;
};

// Returns the active IXInput (production RealXInput or injected fake).
IXInput* xInput();
void setXInputForTesting(IXInput* fake);

// Production wrapper around XInputGetState. Tests inject a fake via
// setXInputForTesting() instead of using this class directly.
class XInputWrapper : public QObject, public IXInput {
    Q_OBJECT
public:
    explicit XInputWrapper(QObject* parent = nullptr);
    bool getState(DWORD userIndex, XINPUT_STATE* state) override;
};
