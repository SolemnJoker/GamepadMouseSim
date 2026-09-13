#include "test_config_profiles.h"
#include "core/Config.h"
#include "core/Types.h"
#include <QFile>

void TestConfigProfiles::init() {
    m_dir = new QTemporaryDir();
    QVERIFY(m_dir->isValid());
    m_config = new Config(this);
    QVERIFY(m_config->load(m_dir->filePath("config.json"))); // 内置默认兜底,version 2
}

void TestConfigProfiles::cleanup() {
    delete m_config;
    m_config = nullptr;
    delete m_dir;
    m_dir = nullptr;
}

void TestConfigProfiles::setActiveProfile_expandsMirror() {
    // 造第二套方案:createProfile 复制"当前生效方案"(完整,来自
    // load 时的 reconcile 采集),随后改 Y 键制造差异。
    QVERIFY(m_config->createProfile(QStringLiteral("media")));
    QVERIFY(!m_config->value("profiles.list.media.mouse_mode").toJsonValue().toObject().isEmpty());

    m_config->setActiveProfile(QStringLiteral("media"));

    m_config->beginBatch();
    m_config->setValue("mouse_mode.button_mapping.Y", QStringLiteral("Ctrl+V"));
    m_config->endBatch();
    m_config->updateActiveProfileFromMouseMode();
    QCOMPARE(m_config->activeProfileName(), QStringLiteral("media"));

    m_config->setActiveProfile(QStringLiteral("default"));
    QCOMPARE(m_config->value("mouse_mode.button_mapping.Y").toString(), QStringLiteral("Enter"));

    m_config->setActiveProfile(QStringLiteral("media"));
    QCOMPARE(m_config->value("mouse_mode.button_mapping.Y").toString(), QStringLiteral("Ctrl+V"));
    // 缺子键补默认:media 方案里 button_mapping 完整
    QCOMPARE(m_config->value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseLeftClick"));
}

// profile 未采集时,手改 mouse_mode 是权威:reconcile 采集进 profile
// 而不是用骨架覆盖 mouse_mode(design D3 的采集方向)。
void TestConfigProfiles::emptyProfile_mouseModeStaysAuthoritative() {
    m_config->setValue("mouse_mode.button_mapping.A", QStringLiteral("MouseRightClick"));
    m_config->reconcileProfileMirror();
    QCOMPARE(m_config->value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseRightClick"));
    QCOMPARE(m_config->value("profiles.list.default.mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseRightClick"));
}

void TestConfigProfiles::createProfile_copiesCurrent_andAppendsOrder() {
    QVERIFY(m_config->createProfile(QStringLiteral("browser")));
    QVERIFY(m_config->createProfile(QStringLiteral("media")));
    QCOMPARE(m_config->profileOrder(), (QStringList() << "default"
                                                      << "browser"
                                                      << "media"));
    // 复制的是"当前生效方案" —— active 仍是 default
    QCOMPARE(m_config->activeProfileName(), QStringLiteral("default"));
    QVERIFY(
        !m_config->value("profiles.list.browser.mouse_mode").toJsonValue().toObject().isEmpty());
    QVERIFY(!m_config->createProfile(QStringLiteral("browser"))); // 重名失败
    QVERIFY(!m_config->createProfile(QStringLiteral("")));        // 空名失败
}

void TestConfigProfiles::removeProfile_keepsAtLeastOne_andSwitchesWhenCurrent() {
    QVERIFY(m_config->createProfile(QStringLiteral("media")));
    QVERIFY(m_config->removeProfile(QStringLiteral("media")));    // 剩两套,可删
    QVERIFY(!m_config->removeProfile(QStringLiteral("default"))); // 最后一套不可删
    QCOMPARE(m_config->profileOrder(), (QStringList() << "default"));

    // 删除当前项 → 自动切到剩余首项并展开
    QVERIFY(m_config->createProfile(QStringLiteral("b")));
    QVERIFY(m_config->createProfile(QStringLiteral("c")));
    m_config->setActiveProfile(QStringLiteral("b"));
    QVERIFY(m_config->removeProfile(QStringLiteral("b")));
    QCOMPARE(m_config->activeProfileName(), QStringLiteral("default"));
    QVERIFY(!m_config->removeProfile(QStringLiteral("不存在的")));
}

void TestConfigProfiles::renameProfile_updatesOrderAndActive() {
    QVERIFY(m_config->createProfile(QStringLiteral("media")));
    QVERIFY(m_config->renameProfile(QStringLiteral("media"), QStringLiteral("player")));
    QCOMPARE(m_config->profileOrder(), (QStringList() << "default"
                                                      << "player"));
    QVERIFY(m_config->value("profiles.list.player").toJsonValue().isObject());
    QVERIFY(!m_config->value("profiles.list.media").toJsonValue().isObject());

    m_config->setActiveProfile(QStringLiteral("player"));
    QVERIFY(m_config->renameProfile(QStringLiteral("player"), QStringLiteral("p2")));
    QCOMPARE(m_config->activeProfileName(), QStringLiteral("p2"));
}

void TestConfigProfiles::updateFromMouseMode_writesBackToActive_only() {
    QVERIFY(m_config->createProfile(QStringLiteral("media")));
    m_config->setActiveProfile(QStringLiteral("media"));

    m_config->beginBatch();
    m_config->setValue("mouse_mode.button_mapping.A", QStringLiteral("MouseRightClick"));
    m_config->endBatch();
    m_config->updateActiveProfileFromMouseMode();

    // 回写到 active(media),default 不受影响(仍是出厂默认 A)
    QCOMPARE(m_config->value("profiles.list.media.mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseRightClick"));
    QCOMPARE(m_config->value("profiles.list.default.mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseLeftClick"));

    // 切回 default → A 恢复默认
    m_config->setActiveProfile(QStringLiteral("default"));
    QCOMPARE(m_config->value("mouse_mode.button_mapping.A").toString(),
             QStringLiteral("MouseLeftClick"));
}

QTEST_MAIN(TestConfigProfiles)
#include "test_config_profiles.moc"
