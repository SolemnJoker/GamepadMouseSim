#pragma once

#include "input/KeyboardController.h"

// 记录型 Fake:替代 ui/KeyboardOverlay,使 KeyboardController 的
// 开合/旁路逻辑无需真实 QWidget 即可测试(design.md D9)。
class FakeOverlay : public IKeyboardOverlay {
  public:
    void showKeyboard() override {
        visible = true;
        ++showCount;
    }
    void hideKeyboard() override {
        visible = false;
        ++hideCount;
    }
    bool isKeyboardVisible() const override { return visible; }

    bool visible = false;
    int showCount = 0;
    int hideCount = 0;
};
