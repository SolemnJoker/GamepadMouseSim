#include "test_config_autostart.h"
#include <QFile>
#include <QJsonParseError>

void TestConfigAutostart::init() {
    m_tmpFile = new QTemporaryFile(this);
    QVERIFY(m_tmpFile->open());
    m_tmpFile->write("{}");
    m_tmpFile->close();

    m_config = new Config(this);
    QVERIFY(m_config->load(m_tmpFile->fileName()));
}

void TestConfigAutostart::cleanup() {
    delete m_config;
    m_config = nullptr;
    delete m_tmpFile;
    m_tmpFile = nullptr;
}

void TestConfigAutostart::setValue_emitsConfigChanged() {
    QSignalSpy spy(m_config, &Config::configChanged);
    m_config->setValue("autostart", true);
    // setValue() writes to disk, which triggers QFileSystemWatcher ->
    // 300 ms debounce -> configChanged().
    QTest::qWait(800);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_config->value("autostart", false).toBool(), true);
}

void TestConfigAutostart::setValue_writesToDisk() {
    m_config->setValue("autostart", true);

    QFile f(m_tmpFile->fileName());
    QVERIFY(f.open(QIODevice::ReadOnly));
    QJsonParseError err;
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll(), &err).object();
    f.close();
    QVERIFY2(err.error == QJsonParseError::NoError, qPrintable(err.errorString()));
    QCOMPARE(obj.value("autostart").toBool(), true);
}

void TestConfigAutostart::reloadReReadsValue() {
    {
        QFile f(m_tmpFile->fileName());
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(R"({"autostart": true})");
        f.close();
    }

    QSignalSpy spy(m_config, &Config::configChanged);
    // QFileSystemWatcher + 300 ms debounce + 350 ms singleShot
    QTest::qWait(800);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_config->value("autostart", false).toBool(), true);
}

void TestConfigAutostart::nestedPathRoundTrip() {
    m_config->setValue("a.b.c", 42);
    m_config->setValue("a.x", "untouched");

    QCOMPARE(m_config->value("a.b.c", -1).toInt(), 42);
    QCOMPARE(m_config->value("a.x").toString(), QStringLiteral("untouched"));

    QJsonObject obj = m_config->value("a").toJsonObject();
    QCOMPARE(obj.value("x").toString(), QStringLiteral("untouched"));
    QCOMPARE(obj.value("b").toObject().value("c").toInt(), 42);
}