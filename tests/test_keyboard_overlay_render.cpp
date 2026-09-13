#include "test_keyboard_overlay_render.h"
#include "FakeKeyInjector.h"
#include "input/KeyboardNavController.h"
#include "ui/KeyboardOverlay.h"

void TestKeyboardOverlayRender::init() {
    m_injector = new FakeKeyInjector();
    m_nav = new KeyboardNavController(*m_injector);
    m_overlay = new KeyboardOverlay();
    m_overlay->setNavController(m_nav);
}

void TestKeyboardOverlayRender::cleanup() {
    delete m_overlay;
    m_overlay = nullptr;
    delete m_nav;
    m_nav = nullptr;
    delete m_injector;
    m_injector = nullptr;
}

void TestKeyboardOverlayRender::grab_matchesLayoutSize_andMovesWithHighlight() {
    m_overlay->showKeyboard();
    QVERIFY(m_overlay->isVisible());

    const QPixmap first = m_overlay->grab();
    QVERIFY(!first.isNull());
    QCOMPARE(first.size(), m_overlay->size());

    // 高亮移动一格 → 画面应发生变化(高亮框位置不同)
    m_nav->setDirection(1, 0);
    m_nav->setDirection(0, 0);
    const QPixmap second = m_overlay->grab();
    QVERIFY(!second.isNull());
    QVERIFY2(first.toImage() != second.toImage(),
             "highlight movement must change the rendered image");

    m_overlay->hideKeyboard();
}

QTEST_MAIN(TestKeyboardOverlayRender)
#include "test_keyboard_overlay_render.moc"
