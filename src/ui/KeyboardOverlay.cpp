#include "KeyboardOverlay.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <windows.h>

KeyboardOverlay::KeyboardOverlay(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool |
                   Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);
}

void KeyboardOverlay::setNavController(KeyboardNavController* nav) {
    m_nav = nav;
    if (m_nav)
        connect(m_nav, &KeyboardNavController::stateChanged, this, qOverload<>(&QWidget::update));
}

void KeyboardOverlay::showKeyboard() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect sg = screen->availableGeometry();
        const QSize size = totalSize();
        move(sg.left() + (sg.width() - size.width()) / 2,
             sg.top() + (sg.height() - size.height()) / 2);
        setFixedSize(size);
    }

    show();
    raise();
    update();

    // 仅诊断日志:打开键盘时的前台窗口(不改变它)。
    HWND fg = GetForegroundWindow();
    if (fg) {
        WCHAR title[256] = {};
        GetWindowTextW(fg, title, 256);
        qDebug() << "Keyboard overlay shown; foreground window:" << QString::fromWCharArray(title);
    }
}

void KeyboardOverlay::hideKeyboard() {
    hide();
}

bool KeyboardOverlay::isKeyboardVisible() const {
    return isVisible();
}

void KeyboardOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (!m_nav)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(20, 22, 28, 215));
    painter.drawRoundedRect(rect(), 12, 12);

    const QFont font = QFont(QStringLiteral("Segoe UI"), 13, QFont::Bold);
    painter.setFont(font);

    for (int r = 0; r < static_cast<int>(m_nav->layout().rows.size()); ++r) {
        const KbRow& row = m_nav->layout().rows[r];
        for (int c = 0; c < static_cast<int>(row.keys.size()); ++c) {
            const KbKey& key = row.keys[c];
            const QRect rect = keyRect(r, c);

            const bool highlighted = (r == m_nav->row() && c == m_nav->col());
            if (highlighted)
                painter.setBrush(QColor(59, 130, 246, 230));
            else
                painter.setBrush(QColor(255, 255, 255, 32));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(rect, 6, 6);

            if (key.kind == KbKey::Kind::Shift && m_nav->shiftLatched()) {
                painter.setPen(QPen(QColor(250, 204, 21), 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
            }

            painter.setPen(QColor(255, 255, 255, key.kind == KbKey::Kind::Close ? 220 : 235));
            painter.drawText(rect, Qt::AlignCenter, key.label);
        }
    }
}

QRect KeyboardOverlay::keyRect(int row, int col) const {
    const KbRow& r = m_nav->layout().rows[row];
    int x = m_gap;
    for (int i = 0; i < col; ++i) {
        x += r.keys[i].width * m_keyUnit + (r.keys[i].width - 1) * m_gap + m_gap;
    }
    const int width = r.keys[col].width * m_keyUnit + (r.keys[col].width - 1) * m_gap;
    const int y = m_gap + row * (m_keyUnit + m_gap);
    return QRect(x, y, width, m_keyUnit);
}

QSize KeyboardOverlay::totalSize() const {
    if (!m_nav)
        return QSize(0, 0);
    const int units = m_nav->layout().maxRowWidthUnits();
    const int rows = static_cast<int>(m_nav->layout().rows.size());
    const int w = m_gap + units * m_keyUnit + (units - 1) * m_gap + m_gap;
    const int h = m_gap + rows * m_keyUnit + (rows - 1) * m_gap + m_gap;
    return QSize(w, h);
}
