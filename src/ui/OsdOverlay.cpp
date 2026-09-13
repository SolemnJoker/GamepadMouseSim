#include "OsdOverlay.h"
#include "core/Config.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>

OsdOverlay::OsdOverlay(Config* config, QWidget* parent)
    : QWidget(parent), m_config(config), m_fadeAnimation(this, "windowOpacity") {
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
    QString text = QString("Pad%1: %2")
                       .arg(controllerIndex + 1)
                       .arg(mode == GamepadMode::Mouse ? "Mouse" : "Default");
    qDebug() << "OSD showing:" << text;
    showMessage(text);
}

void OsdOverlay::showHelp() {
    // 帮助内容按"当前生效映射"实时生成(design.md D5):配置热加载或
    // profile 切换后,下一次呼出即为最新,不再使用构建期静态图。
    if (m_config)
        m_helpSections = HelpContent::build(*m_config);
    m_isHelpMode = true;

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    QRect sg = screen->availableGeometry();

    int w = static_cast<int>(sg.width() * 0.85);
    int h = static_cast<int>(sg.height() * 0.85);
    int x = sg.left() + (sg.width() - w) / 2;
    int y = sg.top() + (sg.height() - h) / 2;

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

    if (m_isHelpMode) {
        painter.setBrush(QColor(0, 0, 0, 230));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 12, 12);
        drawHelp(painter, rect());
        return;
    }

    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Segoe UI", 14, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}

// 帮助排版(design D5):三列布局——第 1 列:直接映射 + 方向键 + 摇杆;
// 第 2 列:L3 层;第 3 列:RT 层 + 模式与系统。字号随区域高度缩放,
// 保持"85% 全屏中文帮助"的既有形态。
void OsdOverlay::drawHelp(QPainter& painter, const QRect& rect) const {
    if (m_helpSections.isEmpty())
        return;

    const int padding = rect.width() / 40;
    const int gap = padding / 2;
    const int colWidth = (rect.width() - 2 * padding - 2 * gap) / 3;
    const int contentTop = rect.height() / 8;
    const int contentHeight = rect.height() - contentTop - padding;

    painter.setPen(QColor(100, 200, 255));
    painter.setFont(
        QFont(QStringLiteral("Microsoft YaHei"), qMax(12, rect.height() / 30), QFont::Bold));
    painter.drawText(QRect(padding, padding / 2, rect.width(), contentTop - padding),
                     Qt::AlignVCenter | Qt::AlignHCenter, QStringLiteral("手柄鼠标模拟器"));

    const int columns[3] = {0, 2, 3};
    const int extraForColumn[3][2] = {{1, 4}, {-1, -1}, {5, -1}};

    for (int col = 0; col < 3; ++col) {
        const int x = padding + col * (colWidth + gap);
        int y = contentTop;

        const int idxs[3] = {columns[col], extraForColumn[col][0], extraForColumn[col][1]};
        const int lineHeight = qMax(16, contentHeight / 22);
        const QFont titleFont(QStringLiteral("Microsoft YaHei"), qMax(9, rect.height() / 46),
                              QFont::Bold);
        const QFont textFont(QStringLiteral("Microsoft YaHei"), qMax(8, rect.height() / 52));

        for (int si = 0; si < 3; ++si) {
            const int index = idxs[si];
            if (index < 0 || index >= m_helpSections.size())
                continue;
            const HelpSection& section = m_helpSections.at(index);
            if (section.lines.isEmpty())
                continue;

            painter.setFont(titleFont);
            painter.setPen(QColor(255, 200, 100));
            painter.drawText(QRect(x, y, colWidth, lineHeight * 2),
                             Qt::AlignLeft | Qt::AlignVCenter, section.title);
            y += lineHeight * 2;

            painter.setFont(textFont);
            for (const HelpLine& line : section.lines) {
                painter.setPen(QColor(255, 200, 100));
                painter.drawText(QRect(x, y, colWidth * 45 / 100, lineHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter, line.button);
                painter.setPen(Qt::white);
                painter.drawText(QRect(x + colWidth * 45 / 100, y, colWidth * 55 / 100, lineHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 QStringLiteral("→ ") + line.action);
                y += lineHeight;
            }
            y += lineHeight;
            if (y > rect.height() - padding)
                break;
        }
    }
}
