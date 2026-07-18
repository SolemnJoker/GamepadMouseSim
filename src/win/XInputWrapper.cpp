#include "XInputWrapper.h"

namespace {
IXInput* g_testXInput = nullptr;
}

class RealXInput : public IXInput {
    bool getState(DWORD userIndex, XINPUT_STATE* state) override {
        return XInputGetState(userIndex, state) == ERROR_SUCCESS;
    }
};

IXInput* xInput() {
    if (g_testXInput)
        return g_testXInput;
    static RealXInput real;
    return &real;
}

void setXInputForTesting(IXInput* fake) {
    g_testXInput = fake;
}

XInputWrapper::XInputWrapper(QObject* parent) : QObject(parent) {}

bool XInputWrapper::getState(DWORD userIndex, XINPUT_STATE* state) {
    return XInputGetState(userIndex, state) == ERROR_SUCCESS;
}