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
    void showModeChange(GamepadMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void showMessage(const QString& text);
    QString m_text;
    QPropertyAnimation m_fadeAnimation;
    QTimer m_hideTimer;
};
