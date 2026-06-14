#include "OsdOverlay.h"
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

OsdOverlay::OsdOverlay(QWidget* parent)
    : QWidget(parent)
    , m_fadeAnimation(this, "windowOpacity")
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(300, 60);

    m_fadeAnimation.setDuration(300);
    m_fadeAnimation.setStartValue(0.0);
    m_fadeAnimation.setEndValue(1.0);

    connect(&m_hideTimer, &QTimer::timeout, this, [this]() {
        m_fadeAnimation.stop();
        m_fadeAnimation.setDirection(QPropertyAnimation::Backward);
        m_fadeAnimation.start();
    });

    connect(&m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_fadeAnimation.direction() == QPropertyAnimation::Backward) {
            hide();
        }
    });
}

void OsdOverlay::showModeChange(GamepadMode mode) {
    QString text = (mode == GamepadMode::Mouse) ? "Mouse Mode" : "Default Mode";
    qDebug() << "OSD showing:" << text;
    showMessage(text);
}

void OsdOverlay::showMessage(const QString& text) {
    m_text = text;

    m_hideTimer.stop();
    m_fadeAnimation.stop();

    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.right() - width() - 20, screenGeometry.bottom() - height() - 20);
    }

    setWindowOpacity(0.0);
    show();
    raise();
    update();

    m_fadeAnimation.setDirection(QPropertyAnimation::Forward);
    m_fadeAnimation.start();

    m_hideTimer.start(2000);
}

void OsdOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(QColor(0, 0, 0, 160));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Segoe UI", 14, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}
