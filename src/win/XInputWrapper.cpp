#include "XInputWrapper.h"

XInputWrapper::XInputWrapper(QObject* parent)
    : QObject(parent)
{
}

bool XInputWrapper::getState(DWORD userIndex, XINPUT_STATE* state) {
    return XInputGetState(userIndex, state) == ERROR_SUCCESS;
}
