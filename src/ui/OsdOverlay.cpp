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

void OsdOverlay::showHelp(const QStringList& lines) {
    m_isHelpMode = true;

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QRect sg = screen->availableGeometry();
    int w = static_cast<int>(sg.width() * 0.85);
    int h = static_cast<int>(sg.height() * 0.85);
    int x = sg.left() + (sg.width() - w) / 2;
    int y = sg.top() + (sg.height() - h) / 2;

    QFont font("Microsoft YaHei", 25);
    QFontMetrics fm(font);
    int lineHeight = fm.height() + 8;
    int margin = 40;
    int availWidth = w - margin * 2;
    int availHeight = h - margin * 2;
    int linesPerColumn = qMax(1, availHeight / lineHeight);
    int totalLines = lines.size();
    int numColumns = qMax(1, (totalLines + linesPerColumn - 1) / linesPerColumn);
    int colWidth = availWidth / numColumns;
    int arrowX = 180;

    m_helpPixmap = QPixmap(w, h);
    m_helpPixmap.fill(QColor(0, 0, 0, 200));
    QPainter p(&m_helpPixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setFont(font);
    for (int col = 0; col < numColumns; ++col) {
        int startLine = col * linesPerColumn;
        int endLine = qMin(startLine + linesPerColumn, totalLines);
        int px = margin + col * colWidth;
        for (int i = startLine; i < endLine; ++i) {
            int py = margin + (i - startLine) * lineHeight + fm.ascent();
            QString line = lines[i];
            QColor c = Qt::white;
            if (i > 0 && lines[i - 1].isEmpty()) {
                c = QColor(255, 200, 100);
            } else if (i == 0) {
                c = QColor(100, 200, 255);
            }
            p.setPen(c);

            int sepIdx = line.indexOf("→");
            if (sepIdx > 0) {
                QString leftPart = line.left(sepIdx).trimmed();
                QString rightPart = line.mid(sepIdx + 1).trimmed();
                int ax = px + arrowX;
                p.drawText(px, py, leftPart);
                p.drawText(px + ax, py, "→");
                p.drawText(px + ax + fm.horizontalAdvance("→  "), py, rightPart);
            } else {
                p.drawText(px, py, line);
            }
        }
    }
    p.end();

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

    if (m_isHelpMode && !m_helpPixmap.isNull()) {
        painter.drawPixmap(0, 0, m_helpPixmap);
    } else if (!m_isHelpMode) {
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 14, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, m_text);
    }
}
