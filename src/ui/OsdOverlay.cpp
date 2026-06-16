#include "OsdOverlay.h"
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QTextOption>
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
            m_isHelpMode = false;
        }
    });
}

void OsdOverlay::showModeChange(int controllerIndex, GamepadMode mode) {
    QString text = QString("Pad%1: %2").arg(controllerIndex + 1)
                   .arg(mode == GamepadMode::Mouse ? "Mouse" : "Default");
    qDebug() << "OSD showing:" << text;
    showMessage(text);
}

void OsdOverlay::showHelp(const QString& text) {
    m_isHelpMode = true;

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        showMessage(text, 5000);
        return;
    }
    QRect sg = screen->availableGeometry();
    int w = static_cast<int>(sg.width() * 0.85);
    int h = static_cast<int>(sg.height() * 0.85);
    int x = sg.left() + (sg.width() - w) / 2;
    int y = sg.top() + (sg.height() - h) / 2;

    m_text = text;
    m_hideTimer.stop();
    m_fadeAnimation.stop();

    setFixedSize(w, h);
    move(x, y);

    setWindowOpacity(0.0);
    show();
    raise();
    update();

    m_fadeAnimation.setDirection(QPropertyAnimation::Forward);
    m_fadeAnimation.start();

    m_hideTimer.start(5000);
}

void OsdOverlay::showMessage(const QString& text, int durationMs, int width, int height) {
    m_text = text;

    m_hideTimer.stop();
    m_fadeAnimation.stop();

    setFixedSize(width, height);

    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.right() - width - 20, screenGeometry.bottom() - height - 20);
    }

    setWindowOpacity(0.0);
    show();
    raise();
    update();

    m_fadeAnimation.setDirection(QPropertyAnimation::Forward);
    m_fadeAnimation.start();

    m_hideTimer.start(durationMs);
}

void OsdOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);

    painter.setPen(Qt::white);
    if (m_isHelpMode) {
        QFont font("Microsoft YaHei", 14);
        painter.setFont(font);
        QTextOption opt;
        opt.setWrapMode(QTextOption::WordWrap);
        painter.drawText(rect().adjusted(25, 25, -25, -25), m_text, opt);
    } else {
        painter.setFont(QFont("Segoe UI", 14, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, m_text);
    }
}
