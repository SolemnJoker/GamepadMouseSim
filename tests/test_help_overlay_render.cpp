#include "test_help_overlay_render.h"
#include "core/Config.h"
#include "ui/OsdOverlay.h"
#include <QTemporaryDir>

void TestHelpOverlayRender::init() {
    m_dir = new QTemporaryDir();
    QVERIFY(m_dir->isValid());
    m_config = new Config(this);
    QVERIFY(m_config->load(m_dir->filePath("config.json")));
    m_overlay = new OsdOverlay(m_config);
}

void TestHelpOverlayRender::cleanup() {
    delete m_overlay;
    m_overlay = nullptr;
    delete m_config;
    m_config = nullptr;
    delete m_dir;
    m_dir = nullptr;
}

void TestHelpOverlayRender::grab_rendersHelpContent_andFollowsConfig() {
    m_overlay->showHelp();
    QVERIFY(m_overlay->isVisible());
    QTest::qWait(450); // 等淡入动画走完,避免全透明像素干扰断言

    const QImage first = m_overlay->grab().toImage();
    QVERIFY(!first.isNull());
    QCOMPARE(first.size(), m_overlay->size());

    // 修改映射配置 → 下一次 showHelp 的画面必须变化(帮助实时化契约)
    m_config->setValue("mouse_mode.button_mapping.A", QStringLiteral("MouseRightClick"));
    m_overlay->showHelp();
    QTest::qWait(450);
    const QImage second = m_overlay->grab().toImage();
    QCOMPARE(second.size(), m_overlay->size());
    QVERIFY2(first != second, "help screen must re-render when config changes");
}

QTEST_MAIN(TestHelpOverlayRender)
#include "test_help_overlay_render.moc"
