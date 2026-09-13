#pragma once

#include <QtTest>

class FakeKeyInjector;
class KeyboardNavController;
class KeyboardOverlay;

// 虚拟键盘 overlay 的离屏渲染断言(design.md D7 L3):键位网格渲染、
// 高亮移动后画面变化。
class TestKeyboardOverlayRender : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void grab_matchesLayoutSize_andMovesWithHighlight();

  private:
    FakeKeyInjector* m_injector = nullptr;
    KeyboardNavController* m_nav = nullptr;
    KeyboardOverlay* m_overlay = nullptr;
};
