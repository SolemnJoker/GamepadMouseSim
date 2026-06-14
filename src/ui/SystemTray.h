#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <array>
#include "core/Types.h"

class SystemTray : public QObject {
    Q_OBJECT
public:
    explicit SystemTray(QObject* parent = nullptr);

    void show();
    void hide();

public slots:
    void onModeChanged(int controllerIndex, GamepadMode mode);

signals:
    void switchModeRequested();
    void lockModeRequested();
    void pauseRequested();
    void exitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void updateIcon(GamepadMode mode);
    void updateTooltip();

    QSystemTrayIcon m_trayIcon;
    QMenu m_menu;
    QAction* m_statusAction;
    QAction* m_switchAction;
    QAction* m_lockAction;
    QAction* m_pauseAction;
    std::array<GamepadMode, kMaxGamepads> m_padModes;
    std::array<bool, kMaxGamepads> m_padConnected = {};
};
