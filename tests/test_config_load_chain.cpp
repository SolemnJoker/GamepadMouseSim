#include "test_config_load_chain.h"
#include "core/Config.h"
#include "core/Types.h"
#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>

void TestConfigLoadChain::init() {
    m_dir = new QTemporaryDir();
    QVERIFY(m_dir->isValid());
    m_config = new Config(this);
}

void TestConfigLoadChain::cleanup() {
    delete m_config;
    m_config = nullptr;
    delete m_dir;
    m_dir = nullptr;
}

void TestConfigLoadChain::writeRaw(const QByteArray& json) {
    QFile f(m_dir->filePath("config.json"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();
}

// 裸 exe 首启动:目录里无任何文件 → qrc 内嵌默认兜底 → 自动写出可写
// config.json,数据完整(design D1;spec:配置加载链兜底)。
void TestConfigLoadChain::bareExe_firstLaunch_generatesWritableConfig() {
    const QString path = m_dir->filePath("config.json");
    QVERIFY(!QFile::exists(path));

    QVERIFY(m_config->load(path)); // qrc 兜底成功即视为加载成功

    QVERIFY2(QFile::exists(path), "writable config.json must be generated on first launch");
    QCOMPARE(m_config->value("schema_version").toInt(), kCurrentConfigSchemaVersion);
    QVERIFY(!m_config->value("mouse_mode.button_mapping.A").toString().isEmpty());
    QVERIFY(!m_config->profileOrder().isEmpty());
    QCOMPARE(m_config->activeProfileName(), QStringLiteral("default"));
}

// v1 旧配置升级:用户自定义保留、缺失键补默认、mouse_mode 原样成为
// profiles.list.default(design D4;spec:增量合并升级)。
void TestConfigLoadChain::legacyV1Config_incrementalMerge_preservesCustom() {
    writeRaw(R"({
        "schema_version": 1,
        "mouse_mode": {
            "button_mapping": {"A": "MouseRightClick", "B": "None"},
            "modifier_mapping": {"LT": {"Menu": "None"}}
        },
        "osd": {"enabled": false}
    })");

    QVERIFY(m_config->load(m_dir->filePath("config.json")));
    QCOMPARE(m_config->value("schema_version").toInt(), 2);

    // 用户自定义保留
    QCOMPARE(m_config->value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseRightClick"));
    // 显式 None 保留(用户意图)
    QCOMPARE(m_config->value("mouse_mode.button_mapping.B").toString(), QStringLiteral("None"));
    QCOMPARE(m_config->value("mouse_mode.modifier_mapping.LT.Menu").toString(),
             QStringLiteral("None"));
    // 缺失键补默认
    QCOMPARE(m_config->value("mouse_mode.button_mapping.X").toString(),
             QStringLiteral("MouseMiddleClick"));
    // 未涉及节保留
    QCOMPARE(m_config->value("osd.enabled").toBool(), false);
    // profile 化
    QVERIFY(
        !m_config->value("profiles.list.default.mouse_mode").toJsonValue().toObject().isEmpty());
    QCOMPARE(m_config->activeProfileName(), QStringLiteral("default"));
}

// 坏 JSON:回退出厂默认并把文件重写为有效内容(spec:坏 JSON 回退)。
void TestConfigLoadChain::corruptJson_resetsToFactory_andRewritesValidFile() {
    writeRaw("this is not json {{{");

    QVERIFY(m_config->load(m_dir->filePath("config.json")));
    QCOMPARE(m_config->value("schema_version").toInt(), kCurrentConfigSchemaVersion);
    QVERIFY(!m_config->value("mouse_mode.button_mapping.A").toString().isEmpty());

    QFile f(m_dir->filePath("config.json"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray rewritten = f.readAll();
    f.close();
    QVERIFY(!rewritten.contains("this is not json"));
    QVERIFY(QJsonDocument::fromJson(rewritten).isObject());
}

// 用户主动一键恢复出厂:整个配置替换为内置默认并落盘重写。
void TestConfigLoadChain::restoreFactoryDefaults_replacesUserKeys_andRewritesFile() {
    QVERIFY(m_config->load(m_dir->filePath("config.json")));
    m_config->setValue("autostart", true);
    m_config->setValue("mouse_mode.button_mapping.A", QStringLiteral("MouseRightClick"));
    QSignalSpy spy(m_config, &Config::configChanged);

    m_config->restoreFactoryDefaults();

    QCOMPARE(m_config->value("autostart", false).toBool(), false);
    QCOMPARE(m_config->value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseLeftClick"));
    QCOMPARE(m_config->value("schema_version").toInt(), kCurrentConfigSchemaVersion);
    QVERIFY(spy.count() >= 1); // 热广播立即发出

    // 落盘重写:新实例加载同一文件得到出厂值
    Config fresh(this);
    QVERIFY(fresh.load(m_dir->filePath("config.json")));
    QCOMPARE(fresh.value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseLeftClick"));
}

// v2 合法配置:load 不得动 mouse_mode(用户手改值是权威,profile 未采集)。
void TestConfigLoadChain::existingV2Config_loadDoesNotTouchMouseMode() {
    writeRaw(R"({
        "schema_version": 2,
        "profiles": {"active": "default", "order": ["default"], "list": {"default": {}}},
        "mouse_mode": {"button_mapping": {"A": "MouseRightClick"}}
    })");

    QVERIFY(m_config->load(m_dir->filePath("config.json")));
    QCOMPARE(m_config->value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseRightClick"));
}

QTEST_MAIN(TestConfigLoadChain)
#include "test_config_load_chain.moc"
