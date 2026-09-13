#pragma once

#include "core/AutoStart.h"
#include <QHash>
#include <QString>
#include <QtTest>

class FakeRegistry : public IRegistry {
  public:
    bool contains(const QString& v) const override { return values.contains(v); }
    QString value(const QString& v) const override { return values.value(v); }
    void setValue(const QString& v, const QString& val) override {
        lastWrite = v;
        values[v] = val;
    }
    void remove(const QString& v) override { values.remove(v); }
    void sync() override { ++syncCount; }

    QHash<QString, QString> values;
    QString lastWrite;
    int syncCount = 0;
};

class TestAutostartApply : public QObject {
    Q_OBJECT
  private slots:
    void init() {
        m_fake.values.clear();
        m_fake.lastWrite.clear();
        m_fake.syncCount = 0;
        setRegistryForTesting(&m_fake);
    }
    void cleanup() { setRegistryForTesting(nullptr); }

    void enable_writesQuotedPath();
    void enable_whenAlreadyEnabled_idempotent();
    void disable_whenAbsent_noop();
    void disable_removesOwnRegistration();
    void disable_removesDeadPathResidue();
    void disable_keepsOtherLiveRegistration();
    void runValuePointsToCurrentExe_matchesOnlyOwnPath();

  private:
    FakeRegistry m_fake;
};
