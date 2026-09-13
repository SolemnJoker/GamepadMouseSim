#pragma once

#include "core/Types.h"
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <array>

// 纯函数:把 profile 列表装进 QMenu(不触碰 QSystemTrayIcon,可离屏单测,
// 规避 tray-icon 测试的 offscreen 崩溃先例)。当前项打勾。
void fillProfileMenu(QMenu* menu, const QStringList& order, const QString& active);

class SystemTray : public QObject {
    Q_OBJECT
  public:
    explicit SystemTray(QObject* parent = nullptr);

    void show();
    void hide();

    QIcon trayIcon() const { return m_trayIcon.icon(); }

    void rebuildProfileMenu(const QStringList& order, const QString& active);

  public slots:
    void onModeChanged(int controllerIndex, GamepadMode mode);

  signals:
    void switchModeRequested();
    void lockModeRequested();
    void pauseRequested();
    void exitRequested();
    void settingsRequested();
    void profileSwitchRequested(const QString& name);
    void restoreDefaultsRequested();

  private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

  private:
    void updateIcon(GamepadMode mode);
    void updateTooltip();
    static QIcon renderSvg(const QString& path, int size);

    QSystemTrayIcon m_trayIcon;
    QMenu m_menu;
    QMenu m_profileMenu;
    QAction* m_statusAction;
    QAction* m_switchAction;
    QAction* m_lockAction;
    QAction* m_pauseAction;
    QAction* m_settingsAction;
    QAction* m_profileMenuAction = nullptr;
    std::array<GamepadMode, kMaxGamepads> m_padModes;
    std::array<bool, kMaxGamepads> m_padConnected = {};
    QIcon m_iconMouse;
    QIcon m_iconGamepad;
};
