#include "SystemTray.h"
#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>

SystemTray::SystemTray(QObject* parent) : QObject(parent) {
    std::fill(m_padModes.begin(), m_padModes.end(), GamepadMode::Default);
    std::fill(m_padConnected.begin(), m_padConnected.end(), false);

    m_statusAction = m_menu.addAction("模式: 默认");
    m_statusAction->setEnabled(false);
    m_menu.addSeparator();

    m_settingsAction = m_menu.addAction(QStringLiteral("设置..."));
    connect(m_settingsAction, &QAction::triggered, this, &SystemTray::settingsRequested);

    m_switchAction = m_menu.addAction("切换模式");
    connect(m_switchAction, &QAction::triggered, this, &SystemTray::switchModeRequested);

    m_lockAction = m_menu.addAction("锁定模式");
    m_lockAction->setCheckable(true);
    connect(m_lockAction, &QAction::triggered, this, &SystemTray::lockModeRequested);

    m_pauseAction = m_menu.addAction("暂停映射");
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::triggered, this, &SystemTray::pauseRequested);

    m_menu.addSeparator();

    QAction* exitAction = m_menu.addAction("退出");
    connect(exitAction, &QAction::triggered, this, &SystemTray::exitRequested);

    m_trayIcon.setContextMenu(&m_menu);
    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &SystemTray::onActivated);

    m_iconMouse = renderSvg(":/icons/mouse.svg", 32);
    m_iconGamepad = renderSvg(":/icons/gamepad.svg", 32);
    updateIcon(GamepadMode::Default);
}

void SystemTray::show() {
    m_trayIcon.show();
}

void SystemTray::hide() {
    m_trayIcon.hide();
}

void SystemTray::onModeChanged(int controllerIndex, GamepadMode mode) {
    if (controllerIndex >= kMaxGamepads)
        return;
    m_padModes[controllerIndex] = mode;
    m_padConnected[controllerIndex] = true;

    updateTooltip();

    for (int i = 0; i < kMaxGamepads; ++i) {
        if (m_padConnected[i]) {
            updateIcon(m_padModes[i]);
            break;
        }
    }
}

void SystemTray::onActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        emit switchModeRequested();
    }
}

void SystemTray::updateTooltip() {
    QStringList padInfo;
    for (int i = 0; i < kMaxGamepads; ++i) {
        if (m_padConnected[i]) {
            padInfo << QString("P%1:%2").arg(i + 1).arg(m_padModes[i] == GamepadMode::Mouse ? "M"
                                                                                            : "D");
        }
    }
    if (padInfo.isEmpty()) {
        m_trayIcon.setToolTip("Gamepad Mouse Simulator");
        m_statusAction->setText("No gamepads");
    } else {
        m_trayIcon.setToolTip("GamepadMouseSim - " + padInfo.join(" "));
        m_statusAction->setText(padInfo.join("  "));
    }
}

void SystemTray::updateIcon(GamepadMode mode) {
    m_trayIcon.setIcon(mode == GamepadMode::Mouse ? m_iconMouse : m_iconGamepad);
}

QIcon SystemTray::renderSvg(const QString& path, int size) {
    QSvgRenderer renderer(path);
    if (!renderer.isValid())
        return QIcon(path);
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter);
    painter.end();
    return QIcon(pixmap);
}
