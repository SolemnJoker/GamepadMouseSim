#pragma once

#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "core/Config.h"

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