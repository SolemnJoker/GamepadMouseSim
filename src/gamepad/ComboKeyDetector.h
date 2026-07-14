#pragma once

#include "core/Types.h"
#include <QElapsedTimer>
#include <QObject>

class ComboKeyDetector : public QObject {
    Q_OBJECT
  public:
    explicit ComboKeyDetector(int controllerIndex, QObject* parent = nullptr);

    void setHoldDuration(int ms) { m_holdDurationMs = ms; }

  public slots:
    void onGamepadState(int controllerIndex, const GamepadState& state);

  signals:
    void comboTriggered(int controllerIndex);

  private:
    int m_controllerIndex;
    QElapsedTimer m_holdTimer;
    bool m_holding = false;
    int m_holdDurationMs = kDefaultComboHoldMs;
};
