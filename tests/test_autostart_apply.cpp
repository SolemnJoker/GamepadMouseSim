#include "test_autostart_apply.h"
#include <QFile>

void TestAutostartApply::enable_writesQuotedPath() {
    AutoStart::applyToRegistry(true);
    QVERIFY(m_fake.contains(AutoStart::kValueName));
    const QString written = m_fake.values.value(AutoStart::kValueName);
    QVERIFY2(written.startsWith('"') && written.endsWith('"'),
             qPrintable(QString("expected quoted path, got: %1").arg(written)));
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

// 移除"指向本 exe"的注册(自己关闭自己)
void TestAutostartApply::disable_removesOwnRegistration() {
    const QString own = "\"" + AutoStart::executablePath() + "\"";
    m_fake.values[AutoStart::kValueName] = own;
    AutoStart::applyToRegistry(false);
    QVERIFY(!m_fake.contains(AutoStart::kValueName));
}

// 自愈:残留值指向已不存在的路径(目录被挪走/改名/删除)→ 清理
void TestAutostartApply::disable_removesDeadPathResidue() {
    m_fake.values[AutoStart::kValueName] = "\"X:\\ghost\\GamepadMouseSim.exe\"";
    AutoStart::applyToRegistry(false);
    QVERIFY(!m_fake.contains(AutoStart::kValueName));
}

// 保守:残留值指向另一份仍存在的拷贝 → 不动(不是本实例的注册)
void TestAutostartApply::disable_keepsOtherLiveRegistration() {
    const QString live = "\"C:\\Windows\\System32\\cmd.exe\"";
    QVERIFY(QFile::exists("C:\\Windows\\System32\\cmd.exe")); // 前提:参照物存在
    m_fake.values[AutoStart::kValueName] = live;
    AutoStart::applyToRegistry(false);
    QVERIFY(m_fake.contains(AutoStart::kValueName));
    QCOMPARE(m_fake.values.value(AutoStart::kValueName), live);
}

// 启动采集:Run 值指向本 exe(如安装器写入)→ true;其他情形 → false
void TestAutostartApply::runValuePointsToCurrentExe_matchesOnlyOwnPath() {
    m_fake.values[AutoStart::kValueName] = "\"" + AutoStart::executablePath() + "\"";
    QCOMPARE(AutoStart::runValuePointsToCurrentExe(), true);

    m_fake.values[AutoStart::kValueName] = "\"X:\\ghost\\GamepadMouseSim.exe\"";
    QCOMPARE(AutoStart::runValuePointsToCurrentExe(), false);

    m_fake.values.remove(AutoStart::kValueName);
    QCOMPARE(AutoStart::runValuePointsToCurrentExe(), false);
}

#include "test_autostart_apply.moc"
