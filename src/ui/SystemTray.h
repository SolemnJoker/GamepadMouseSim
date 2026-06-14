#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include "core/Types.h"

class SystemTray : public QObject {
    Q_OBJECT
public:
    explicit SystemTray(QObject* parent = nullptr);

    void show();
    void hide();

public slots:
    void onModeChanged(GamepadMode mode);

signals:
    void switchModeRequested();
    void lockModeRequested();
    void pauseRequested();
    void exitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void updateIcon(GamepadMode mode);

    QSystemTrayIcon m_trayIcon;
    QMenu m_menu;
    QAction* m_statusAction;
    QAction* m_switchAction;
    QAction* m_lockAction;
    QAction* m_pauseAction;
    GamepadMode m_mode = GamepadMode::Mouse;
};
