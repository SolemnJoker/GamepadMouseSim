#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include "core/Types.h"

class OsdOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float windowOpacity READ windowOpacity WRITE setWindowOpacity)
public:
    explicit OsdOverlay(QWidget* parent = nullptr);

public slots:
    void showModeChange(int controllerIndex, GamepadMode mode);
    void showHelp(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void showMessage(const QString& text, int durationMs = 2000, int width = 300, int height = 60);
    bool m_isHelpMode = false;
    QString m_text;
    QPropertyAnimation m_fadeAnimation;
    QTimer m_hideTimer;
    int m_hideTimerId = 0;
};
