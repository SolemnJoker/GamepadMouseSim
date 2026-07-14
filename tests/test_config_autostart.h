#pragma once

#include "core/Config.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

class TestConfigAutostart : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void setValue_emitsConfigChanged();
    void setValue_writesToDisk();
    void reloadReReadsValue();
    void nestedPathRoundTrip();

  private:
    QTemporaryFile* m_tmpFile = nullptr;
    Config* m_config = nullptr;
};