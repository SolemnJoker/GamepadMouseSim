#pragma once

#include "input/KeyboardController.h"
#include "input/KeyboardNavController.h"
#include <QWidget>

class QPaintEvent;

// 虚拟键盘 overlay(design.md D5):
// - 无边框、置顶、不激活(WS_EX_NOACTIVATE 经 Qt::WindowDoesNotAcceptFocus);
//   WA_ShowWithoutActivating 保证 show() 不抢前台焦点,文字始终落进
//   用户打开键盘前所在的窗口。
// - 按 KeyboardNavController 的布局数据表绘制键位与高亮。
// - 仅做视图:实现 IKeyboardOverlay 供 KeyboardController 驱动,
//   导航决策全在 KeyboardNavController/KeyboardController。
class KeyboardOverlay : public QWidget, public IKeyboardOverlay {
    Q_OBJECT
  public:
    explicit KeyboardOverlay(QWidget* parent = nullptr);

    // 布局与高亮来源(控制器持有 nav,窗口创建晚于控制器时注入)。
    void setNavController(KeyboardNavController* nav);

    // IKeyboardOverlay
    void showKeyboard() override;
    void hideKeyboard() override;
    bool isKeyboardVisible() const override;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QRect keyRect(int row, int col) const;
    QSize totalSize() const;

    KeyboardNavController* m_nav = nullptr;
    int m_keyUnit = 52; // 标准键位的边长(px,v1 固定缩放)
    int m_gap = 6;      // 键位间距(px)
};
