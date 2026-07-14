#pragma once

#include "core/Types.h"
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <array>

class SystemTray : public QObject {
    Q_OBJECT
  public:
    explicit SystemTray(QObject* parent = nullptr);

    void show();
    void hide();

    QIcon trayIcon() const { return m_trayIcon.icon(); }

  public slots:
    void onModeChanged(int controllerIndex, GamepadMode mode);

  signals:
    void switchModeRequested();
    void lockModeRequested();
    void pauseRequested();
    void exitRequested();
    void settingsRequested();

  private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

  private:
    void updateIcon(GamepadMode mode);
    void updateTooltip();
    static QIcon renderSvg(const QString& path, int size);

    QSystemTrayIcon m_trayIcon;
    QMenu m_menu;
    QAction* m_statusAction;
    QAction* m_switchAction;
    QAction* m_lockAction;
    QAction* m_pauseAction;
    QAction* m_settingsAction;
    std::array<GamepadMode, kMaxGamepads> m_padModes;
    std::array<bool, kMaxGamepads> m_padConnected = {};
    QIcon m_iconMouse;
    QIcon m_iconGamepad;
};
