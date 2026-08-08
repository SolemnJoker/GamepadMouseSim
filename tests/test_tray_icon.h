#pragma once

#include "ui/SystemTray.h"
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QtTest>
#include <memory>

class TestTrayIcon : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase();
    void construct_doesNotThrow();
    void icon_isNotNull();

  private:
    std::unique_ptr<SystemTray> m_tray;
};
