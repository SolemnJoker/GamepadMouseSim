#include "test_keyboard_nav.h"
#include "FakeKeyInjector.h"
#include "input/KeyboardNavController.h"

void TestKeyboardNav::init() {
    m_injector = new FakeKeyInjector();
    m_nav = new KeyboardNavController(*m_injector, this);
    m_nav->setRepeatTimings(400, 150);
}

void TestKeyboardNav::cleanup() {
    delete m_nav;
    m_nav = nullptr;
    delete m_injector;
    m_injector = nullptr;
}

// 单步移动:setDirection 立即走一格,(0,0) 停掉重复定时器。
void TestKeyboardNav::step(int dx, int dy) {
    m_nav->setDirection(dx, dy);
    m_nav->setDirection(0, 0);
}

void TestKeyboardNav::initial_highlightAtOrigin() {
    QCOMPARE(m_nav->row(), 0);
    QCOMPARE(m_nav->col(), 0);
    QCOMPARE(m_nav->shiftLatched(), false);
}

void TestKeyboardNav::horizontal_movementAndRowEndClamp() {
    for (int i = 0; i < 20; ++i) // 远超行尾,验证 clamp
        step(1, 0);
    const int lastCol = static_cast<int>(m_nav->layout().rows[0].keys.size()) - 1;
    QCOMPARE(m_nav->col(), lastCol);

    for (int i = 0; i < 20; ++i)
        step(-1, 0);
    QCOMPARE(m_nav->col(), 0);
}

void TestKeyboardNav::vertical_movementCentersAlignment() {
    step(0, 1);
    QCOMPARE(m_nav->row(), 1);
    QCOMPARE(m_nav->col(), 0); // '1' 中心 0.5 → 'Q' 中心 0.5

    // 数字行最右(退格,宽 2,中心 11)下移 → 行 1 最近中心是 Esc(10.5)。
    for (int i = 0; i < 20; ++i)
        step(1, 0);
    step(0, 1);
    QCOMPARE(m_nav->row(), 2);
    QCOMPARE(m_nav->col(), static_cast<int>(m_nav->layout().rows[2].keys.size()) - 1);
}

void TestKeyboardNav::repeat_firesAfterDelayAndStopsOnRelease() {
    m_nav->setRepeatTimings(1, 1);
    m_nav->setDirection(0, 1); // 立即 1 步 + 定时重复
    QTest::qWait(80);
    m_nav->setDirection(0, 0);
    const int rowAfterRepeat = m_nav->row();
    QVERIFY2(rowAfterRepeat >= 3, "expected several repeat steps during 80 ms");

    QTest::qWait(30);
    QCOMPARE(m_nav->row(), rowAfterRepeat); // 停止后不再走格
}

void TestKeyboardNav::confirm_letterDigitAndSpecialKeys() {
    // 数字行 col3 = '4'
    for (int i = 0; i < 3; ++i)
        step(1, 0);
    m_nav->confirmKey();
    QCOMPARE(m_injector->keys.size(), size_t(1));
    QCOMPARE(m_injector->keys[0].vk, WORD('4'));
    QCOMPARE(m_injector->keys[0].withShift, false);

    // 下移 4 步到空格键(行 4)。注意行 3 的宽 Shift 键让中心列漂移:
    // '4'(3.5)→ E(3.5)→ F(3.5)→ X(3.5)→ Space(中心 4)。
    for (int i = 0; i < 4; ++i)
        step(0, 1);
    QCOMPARE(m_nav->row(), 4);
    m_nav->confirmKey();
    QCOMPARE(m_injector->keys.back().vk, VK_SPACE);

    // 上移 4 步回到数字行,再走到行尾退格键
    for (int i = 0; i < 4; ++i)
        step(0, -1);
    QCOMPARE(m_nav->row(), 0);
    for (int i = 0; i < 20; ++i)
        step(1, 0);
    m_nav->confirmKey();
    QCOMPARE(m_injector->keys.back().vk, VK_BACK);
}

void TestKeyboardNav::sticky_shiftAppliesToNextLetterOnly() {
    // 三次下移到行 3 col 0 = Shift,确认点亮
    for (int i = 0; i < 3; ++i)
        step(0, 1);
    m_nav->confirmKey();
    QCOMPARE(m_nav->shiftLatched(), true);

    // 右移一格 = 'Z':确认后以 Shift+Z 组合注入,且 Shift 自动熄灭
    step(1, 0);
    m_nav->confirmKey();
    QCOMPARE(m_injector->keys.back().vk, WORD('Z'));
    QCOMPARE(m_injector->keys.back().withShift, true);
    QCOMPARE(m_nav->shiftLatched(), false);

    // 再次确认 'Z':无 Shift
    m_nav->confirmKey();
    QCOMPARE(m_injector->keys.back().withShift, false);

    // 重新点亮 Shift(左移回 Shift 键),确认非字母键(Space):
    // Shift 保持点亮,且整个序列中没有任何孤立 Shift 注入
    step(-1, 0);
    m_nav->confirmKey();
    QCOMPARE(m_nav->shiftLatched(), true);
    step(0, 1); // 行 3 → 行 4(Space)
    m_nav->confirmKey();
    QCOMPARE(m_injector->keys.back().vk, VK_SPACE);
    QCOMPARE(m_injector->keys.back().withShift, false);
    QCOMPARE(m_nav->shiftLatched(), true);

    for (const auto& k : m_injector->keys)
        QVERIFY2(k.vk != VK_SHIFT, "isolated Shift key event must never be sent");
}

void TestKeyboardNav::backspace_directBKey() {
    m_nav->backspaceKey();
    QCOMPARE(m_injector->keys.size(), size_t(1));
    QCOMPARE(m_injector->keys[0].vk, VK_BACK);
    QCOMPARE(m_injector->keys[0].withShift, false);
}

void TestKeyboardNav::close_keyEmitsCloseRequested() {
    QSignalSpy spy(m_nav, &KeyboardNavController::closeRequested);
    for (int i = 0; i < 3; ++i) // 行 3
        step(0, 1);
    for (int i = 0; i < 20; ++i) // 行尾 = Close 键
        step(1, 0);
    QCOMPARE(m_nav->layout().rows[3].keys[m_nav->col()].kind, KbKey::Kind::Close);
    m_nav->confirmKey();
    QCOMPARE(spy.count(), 1);
}

void TestKeyboardNav::resetState_restoresOriginAndShift() {
    step(1, 1); // 任意移动
    QVERIFY(m_nav->row() != 0 || m_nav->col() != 0);

    // 点亮 Shift:两次下移(S 中心 1.5 → Shift 中心 1 为最近)
    for (int i = 0; i < 2; ++i)
        step(0, 1);
    QCOMPARE(m_nav->layout().rows[m_nav->row()].keys[m_nav->col()].kind, KbKey::Kind::Shift);
    m_nav->confirmKey();
    QCOMPARE(m_nav->shiftLatched(), true);

    m_nav->resetState();
    QCOMPARE(m_nav->row(), 0);
    QCOMPARE(m_nav->col(), 0);
    QCOMPARE(m_nav->shiftLatched(), false);
}

QTEST_MAIN(TestKeyboardNav)
#include "test_keyboard_nav.moc"
