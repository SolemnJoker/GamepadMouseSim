#pragma once

#include <QObject>
#include <QElapsedTimer>
#include "core/Types.h"

class ComboKeyDetector : public QObject {
    Q_OBJECT
public:
    explicit ComboKeyDetector(QObject* parent = nullptr);

    void setHoldDuration(int ms) { m_holdDurationMs = ms; }

public slots:
    void onGamepadState(const GamepadState& state);

signals:
    void comboTriggered();

private:
    QElapsedTimer m_holdTimer;
    bool m_holding = false;
    int m_holdDurationMs = kDefaultComboHoldMs;
};
