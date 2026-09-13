#pragma once

#include <QtTest>

class Config;
class QTemporaryFile;
class FakeKeyInjector;
class FakeOverlay;
class KeyboardController;
struct GamepadState;

class TestKeyboardController : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void closed_inputNotConsumed_andOverlayStaysHidden();
    void open_bypassesRegularRouting_andNavigates();
    void close_viaBindingRisingEdge();
    void closed_noReopenAfterClose();
    void defaultMode_ignoresToggle();
    void modeSwitch_autoCloses();
    void multipad_sameOverlayInstance();
    void stick_upMovesHighlightUp();

  private:
    GamepadState makeState(uint16_t buttons, uint16_t prevButtons, float leftX = 0, float leftY = 0,
                           float rt = 0) const;

    QTemporaryFile* m_tmpFile = nullptr;
    Config* m_config = nullptr;
    FakeKeyInjector* m_injector = nullptr;
    FakeOverlay* m_overlay = nullptr;
    KeyboardController* m_controller = nullptr;
};
