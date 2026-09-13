#include "test_keyboard_controller.h"
#include "FakeKeyInjector.h"
#include "FakeOverlay.h"
#include "core/Config.h"
#include "core/Types.h"
#include "input/KeyboardController.h"
#include <QTemporaryFile>

void TestKeyboardController::init() {
    // 旧式最小配置:无 LT 层、无 keyboard 节 —— 验证 D8 默认合并路径。
    m_tmpFile = new QTemporaryFile(this);
    QVERIFY(m_tmpFile->open());
    m_tmpFile->write("{}");
    m_tmpFile->close();

    m_config = new Config(this);
    QVERIFY(m_config->load(m_tmpFile->fileName()));

    m_injector = new FakeKeyInjector();
    m_overlay = new FakeOverlay();
    m_controller = new KeyboardController(m_config, *m_injector, m_overlay, this);
    m_controller->onModeChanged(0, GamepadMode::Mouse);
}

void TestKeyboardController::cleanup() {
    delete m_controller;
    m_controller = nullptr;
    delete m_overlay;
    m_overlay = nullptr;
    delete m_injector;
    m_injector = nullptr;
    delete m_config;
    m_config = nullptr;
    delete m_tmpFile;
    m_tmpFile = nullptr;
}

GamepadState TestKeyboardController::makeState(uint16_t buttons, uint16_t prevButtons, float leftX,
                                               float leftY, float rt) const {
    GamepadState state;
    state.connected = true;
    state.buttons = buttons;
    state.prevButtons = prevButtons;
    state.leftX = leftX;
    state.leftY = leftY;
    state.rightTrigger = rt;
    return state;
}

// 键盘未打开:返回 false(常规路由照旧),overlay 不应自己弹出——
// 打开检测由 InputMapper 负责(经 showKeyboardRequested → openOverlay)。
void TestKeyboardController::closed_inputNotConsumed_andOverlayStaysHidden() {
    const bool consumed = m_controller->onGamepadState(
        0, makeState(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_START, 0));
    QCOMPARE(consumed, false);
    QCOMPARE(m_overlay->visible, false);
}

void TestKeyboardController::open_bypassesRegularRouting_andNavigates() {
    m_controller->openOverlay();
    QCOMPARE(m_overlay->visible, true);

    // 十字键右 → 高亮右移一格,输入被消费(旁路 InputMapper)
    const bool consumed = m_controller->onGamepadState(0, makeState(XINPUT_GAMEPAD_DPAD_RIGHT, 0));
    QCOMPARE(consumed, true);
    QCOMPARE(m_controller->nav().col(), 1);

    // A 上升沿 → 注入 row0 col1 = '2'
    m_controller->onGamepadState(0, makeState(XINPUT_GAMEPAD_A, 0));
    QCOMPARE(m_injector->keys.size(), size_t(1));
    QCOMPARE(m_injector->keys[0].vk, WORD('2'));
}

void TestKeyboardController::close_viaBindingRisingEdge() {
    m_controller->openOverlay();
    QCOMPARE(m_overlay->visible, true);

    // L3 按住 + Menu 上升沿 = 默认绑定(LT 层 Menu),关闭
    const bool consumed = m_controller->onGamepadState(
        0, makeState(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_LEFT_THUMB));
    QCOMPARE(consumed, true);
    QCOMPARE(m_overlay->visible, false);
}

void TestKeyboardController::closed_noReopenAfterClose() {
    m_controller->openOverlay();
    m_controller->onGamepadState(
        0, makeState(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_LEFT_THUMB));
    QCOMPARE(m_overlay->visible, false);

    // 关闭后同一输入不再被消费(回到常规路由;打开由 InputMapper 负责)
    const bool consumed = m_controller->onGamepadState(
        0, makeState(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_START, 0));
    QCOMPARE(consumed, false);
    QCOMPARE(m_overlay->visible, false);
}

void TestKeyboardController::defaultMode_ignoresToggle() {
    m_controller->onModeChanged(0, GamepadMode::Default);
    const bool consumed = m_controller->onGamepadState(
        0, makeState(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_START, 0));
    QCOMPARE(consumed, false);
    QCOMPARE(m_overlay->visible, false);

    // Default 模式下 openOverlay 之外的旁路也不生效
    m_controller->openOverlay(); // 显式打开(模拟 InputMapper 触发)
    const bool consumed2 = m_controller->onGamepadState(0, makeState(XINPUT_GAMEPAD_DPAD_RIGHT, 0));
    QCOMPARE(consumed2, false);
    QCOMPARE(m_controller->nav().col(), 0);
}

void TestKeyboardController::modeSwitch_autoCloses() {
    m_controller->openOverlay();
    QCOMPARE(m_overlay->visible, true);

    m_controller->onModeChanged(0, GamepadMode::Default);
    QCOMPARE(m_overlay->visible, false);

    // 关闭后路由恢复:摇杆不再驱动键盘
    const bool consumed = m_controller->onGamepadState(0, makeState(0, 0, 0.9f, 0.0f));
    QCOMPARE(consumed, false);
}

void TestKeyboardController::multipad_sameOverlayInstance() {
    m_controller->onModeChanged(1, GamepadMode::Mouse);
    m_controller->openOverlay(); // 手柄 1 打开
    QCOMPARE(m_overlay->visible, true);

    // 手柄 2 的输入作用于同一键盘实例
    const bool consumed = m_controller->onGamepadState(1, makeState(XINPUT_GAMEPAD_DPAD_RIGHT, 0));
    QCOMPARE(consumed, true);
    QCOMPARE(m_controller->nav().col(), 1);

    // 手柄 2 切到 Default:键盘关闭(任一手柄切出 Mouse 即关)
    m_controller->onModeChanged(1, GamepadMode::Default);
    QCOMPARE(m_overlay->visible, false);
}

// 回归:XInput 原始 Y 轴向上为正,屏幕坐标向下为正 —— 摇杆上推必须
// 让高亮上移(与 MouseMapper 的 Y 取向一致)。
void TestKeyboardController::stick_upMovesHighlightUp() {
    m_controller->openOverlay();

    // 十字键下移一行
    m_controller->onGamepadState(0, makeState(XINPUT_GAMEPAD_DPAD_DOWN, 0));
    QCOMPARE(m_controller->nav().row(), 1);

    // 摇杆上推(leftY = +0.9):高亮回到顶行
    m_controller->onGamepadState(0, makeState(0, XINPUT_GAMEPAD_DPAD_DOWN, 0.0f, 0.9f));
    QCOMPARE(m_controller->nav().row(), 0);

    // 摇杆下推(leftY = -0.9):高亮重新下移
    m_controller->onGamepadState(0, makeState(0, 0, 0.0f, -0.9f));
    QCOMPARE(m_controller->nav().row(), 1);
}

QTEST_MAIN(TestKeyboardController)
#include "test_keyboard_controller.moc"
