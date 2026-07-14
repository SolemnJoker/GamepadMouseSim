#pragma once

#include "core/Types.h"
#include <QObject>

class Config;

class MouseMapper : public QObject {
    Q_OBJECT
  public:
    explicit MouseMapper(Config* config, QObject* parent = nullptr);

    void processLeftStick(float x, float y);
    void processRightStick(float x, float y);

  public slots:
    void onConfigChanged();

  private:
    void loadConfig();

    Config* m_config;
    float m_sensX = kDefaultSensitivityX;
    float m_sensY = kDefaultSensitivityY;
    float m_deadzone = kDefaultDeadzone;
    bool m_acceleration = true;
    float m_scrollSpeedV = kDefaultScrollSpeed;
    float m_scrollSpeedH = kDefaultScrollSpeed;
    float m_xAccum = 0.0f;
    float m_yAccum = 0.0f;
    float m_scrollVAccum = 0.0f;
    float m_scrollHAccum = 0.0f;
};
