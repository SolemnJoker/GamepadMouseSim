#include "MouseMapper.h"
#include "core/Config.h"
#include "win/SendInputHelper.h"
#include <cmath>
#include <QDebug>

MouseMapper::MouseMapper(Config* config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    loadConfig();
}

void MouseMapper::processLeftStick(float x, float y) {
    float mag = sqrtf(x * x + y * y);

    static int callCount = 0;
    if (callCount % 120 == 0) {
        qDebug() << "Stick: x=" << x << "y=" << y << "mag=" << mag;
    }
    callCount++;

    if (mag < m_deadzone) {
        m_xAccum = 0.0f;
        m_yAccum = 0.0f;
        return;
    }

    float norm = (mag - m_deadzone) / (1.0f - m_deadzone);
    if (norm > 1.0f) norm = 1.0f;
    float nx = x / mag;
    float ny = y / mag;

    if (m_acceleration) {
        norm = norm * norm;
    }

    float dx = nx * norm * m_sensX * 20.0f;
    float dy = -ny * norm * m_sensY * 20.0f;

    m_xAccum += dx;
    m_yAccum += dy;

    int pixX = static_cast<int>(m_xAccum);
    int pixY = static_cast<int>(m_yAccum);
    m_xAccum -= pixX;
    m_yAccum -= pixY;

    if (pixX != 0 || pixY != 0) {
        qDebug() << "Mouse move:" << pixX << pixY;
        SendInputHelper::moveMouse(pixX, pixY);
    }
}

void MouseMapper::processRightStick(float x, float y) {
    float mag = sqrtf(x * x + y * y);
    if (mag < m_deadzone) {
        m_scrollVAccum = 0.0f;
        m_scrollHAccum = 0.0f;
        return;
    }

    m_scrollVAccum += y * m_scrollSpeedV;
    m_scrollHAccum += x * m_scrollSpeedH;

    int vTicks = static_cast<int>(m_scrollVAccum);
    int hTicks = static_cast<int>(m_scrollHAccum);
    m_scrollVAccum -= vTicks;
    m_scrollHAccum -= hTicks;

    if (vTicks != 0) {
        SendInputHelper::scrollVertical(vTicks);
    }
    if (hTicks != 0) {
        SendInputHelper::scrollHorizontal(hTicks);
    }
}

void MouseMapper::onConfigChanged() {
    loadConfig();
}

void MouseMapper::loadConfig() {
    m_sensX = m_config->value("mouse_mode.left_stick.sensitivity_x", kDefaultSensitivityX).toFloat();
    m_sensY = m_config->value("mouse_mode.left_stick.sensitivity_y", kDefaultSensitivityY).toFloat();
    m_deadzone = m_config->value("mouse_mode.left_stick.deadzone", kDefaultDeadzone).toFloat();
    m_acceleration = m_config->value("mouse_mode.left_stick.acceleration", true).toBool();
    m_scrollSpeedV = m_config->value("mouse_mode.right_stick.scroll_speed_vertical", kDefaultScrollSpeed).toFloat();
    m_scrollSpeedH = m_config->value("mouse_mode.right_stick.scroll_speed_horizontal", kDefaultScrollSpeed).toFloat();
}
