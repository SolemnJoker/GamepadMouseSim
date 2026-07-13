#pragma once

#include <QtTest>
#include <QObject>
#include <QIcon>
#include <QPixmap>
#include <memory>
#include "ui/SystemTray.h"

class TestTrayIcon : public QObject {
    Q_OBJECT
private slots:
    void construct_doesNotThrow();
    void icon_isNotNull();
    void trayVisible_reflectsQSystemTrayIcon();

private:
    std::unique_ptr<SystemTray> m_tray;
};
