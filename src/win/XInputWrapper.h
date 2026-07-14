#pragma once

#include <QObject>
#include <windows.h>
#include <xinput.h>

class XInputWrapper : public QObject {
    Q_OBJECT
  public:
    explicit XInputWrapper(QObject* parent = nullptr);

    bool getState(DWORD userIndex, XINPUT_STATE* state);
};
