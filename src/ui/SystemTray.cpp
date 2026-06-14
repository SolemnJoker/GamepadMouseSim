#include "SystemTray.h"
#include <QApplication>
#include <QPainter>

SystemTray::SystemTray(QObject* parent)
    : QObject(parent)
{
    std::fill(m_padModes.begin(), m_padModes.end(), GamepadMode::Mouse);
    std::fill(m_padConnected.begin(), m_padConnected.end(), false);

    m_statusAction = m_menu.addAction("Mode: Mouse");
    m_statusAction->setEnabled(false);
    m_menu.addSeparator();

    m_switchAction = m_menu.addAction("Switch Mode");
    connect(m_switchAction, &QAction::triggered, this, &SystemTray::switchModeRequested);

    m_lockAction = m_menu.addAction("Lock Mode");
    m_lockAction->setCheckable(true);
    connect(m_lockAction, &QAction::triggered, this, &SystemTray::lockModeRequested);

    m_pauseAction = m_menu.addAction("Pause Passthrough");
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::triggered, this, &SystemTray::pauseRequested);

    m_menu.addSeparator();

    QAction* exitAction = m_menu.addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &SystemTray::exitRequested);

    m_trayIcon.setContextMenu(&m_menu);
    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &SystemTray::onActivated);

    updateIcon(GamepadMode::Mouse);
}

void SystemTray::show() {
    m_trayIcon.show();
}

void SystemTray::hide() {
    m_trayIcon.hide();
}

void SystemTray::onModeChanged(int controllerIndex, GamepadMode mode) {
    if (controllerIndex >= kMaxGamepads) return;
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
            padInfo << QString("P%1:%2").arg(i + 1)
                       .arg(m_padModes[i] == GamepadMode::Mouse ? "M" : "D");
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
    QIcon icon;
    if (mode == GamepadMode::Mouse) {
        QPixmap pixmap(16, 16);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.setBrush(Qt::white);
        painter.drawEllipse(6, 2, 4, 6);
        painter.drawLine(8, 8, 8, 14);
        painter.drawLine(8, 14, 5, 12);
        painter.drawLine(8, 14, 11, 12);
        icon = QIcon(pixmap);
    } else {
        QPixmap pixmap(16, 16);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(2, 4, 12, 8, 2, 2);
        painter.drawEllipse(5, 6, 2, 2);
        painter.drawEllipse(9, 6, 2, 2);
        icon = QIcon(pixmap);
    }
    m_trayIcon.setIcon(icon);
}
