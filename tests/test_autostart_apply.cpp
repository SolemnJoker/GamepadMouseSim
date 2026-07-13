#include "test_autostart_apply.h"

void TestAutostartApply::enable_writesQuotedPath() {
    AutoStart::applyToRegistry(true);
    QVERIFY(m_fake.contains(AutoStart::kValueName));
    const QString written = m_fake.values.value(AutoStart::kValueName);
    QVERIFY2(written.startsWith('"') && written.endsWith('"'),
             qPrintable(QString("expected quoted path, got: %1").arg(written)));
    QCOMPARE(m_fake.syncCount, 1);
}

void TestAutostartApply::disable_removesExisting() {
    m_fake.values[AutoStart::kValueName] = "\"X\"";
    AutoStart::applyToRegistry(false);
    QVERIFY(!m_fake.contains(AutoStart::kValueName));
    QCOMPARE(m_fake.syncCount, 1);
}

void TestAutostartApply::enable_whenAlreadyEnabled_idempotent() {
    AutoStart::applyToRegistry(true);
    AutoStart::applyToRegistry(true);
    QCOMPARE(m_fake.syncCount, 2);
    QVERIFY(m_fake.contains(AutoStart::kValueName));
}

void TestAutostartApply::disable_whenAbsent_noop() {
    AutoStart::applyToRegistry(false);
    QCOMPARE(m_fake.syncCount, 1);
}