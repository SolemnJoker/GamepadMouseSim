#pragma once

#include <QObject>
#include "core/Types.h"
#include "MouseMapper.h"
#include "KeyboardMapper.h"

class Config;

class InputMapper : public QObject {
    Q_OBJECT
public:
    explicit InputMapper(Config* config, QObject* parent = nullptr);

public slots:
    void onGamepadStateChanged(const GamepadState& state);
    void onModeChanged(GamepadMode mode);
    void onConfigChanged();

private:
    Config* m_config;
    MouseMapper m_mouseMapper;
    KeyboardMapper m_keyboardMapper;
    GamepadMode m_mode = GamepadMode::Mouse;
    float m_prevLeftTrigger = 0.0f;
    float m_prevRightTrigger = 0.0f;
};
