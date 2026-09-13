#pragma once

#include <QtTest>

class FakeKeyInjector;
class KeyboardNavController;

class TestKeyboardNav : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void initial_highlightAtOrigin();
    void horizontal_movementAndRowEndClamp();
    void vertical_movementCentersAlignment();
    void repeat_firesAfterDelayAndStopsOnRelease();
    void confirm_letterDigitAndSpecialKeys();
    void sticky_shiftAppliesToNextLetterOnly();
    void backspace_directBKey();
    void close_keyEmitsCloseRequested();
    void resetState_restoresOriginAndShift();

  private:
    void step(int dx, int dy);

    FakeKeyInjector* m_injector = nullptr;
    KeyboardNavController* m_nav = nullptr;
};
