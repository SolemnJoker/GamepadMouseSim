#pragma once

#include "KeyboardMapper.h"
#include "MouseMapper.h"
#include "core/Types.h"
#include <QObject>

class Config;

class InputMapper : public QObject {
    Q_OBJECT
  public:
    explicit InputMapper(Config* config, int controllerIndex, QObject* parent = nullptr);

  public slots:
    void onGamepadStateChanged(int controllerIndex, const GamepadState& state);
    void onModeChanged(GamepadMode mode);
    void onConfigChanged();

  signals:
    void showHelpRequested();

  private:
    Config* m_config;
    int m_controllerIndex;
    MouseMapper m_mouseMapper;
    KeyboardMapper m_keyboardMapper;
    GamepadMode m_mode = GamepadMode::Default;
    float m_prevLeftTrigger = 0.0f;
    float m_prevRightTrigger = 0.0f;
};
