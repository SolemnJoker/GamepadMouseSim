#pragma once

#include "KeyInjector.h"
#include "core/Types.h"
#include <QObject>
#include <QTimer>
#include <vector>

// 单个键位:显示标签、注入用 VK、类型与导航布局中的相对宽度。
struct KbKey {
    enum class Kind { Char, Space, Backspace, Enter, Escape, Shift, Close };
    QString label;
    WORD vk = 0;
    Kind kind = Kind::Char;
    int width = 1; // 相对宽度,1 = 标准键位
};

struct KbRow {
    std::vector<KbKey> keys;
};

// 键盘布局(design.md D7):数据驱动,QWERTY + 数字行 + 底部特殊键。
struct KeyboardLayout {
    std::vector<KbRow> rows;

    int maxRowWidthUnits() const;
};

// 默认布局:字母/数字/空格/退格/Esc/回车/Shift/关闭。
// 数字行 + 空格 + Esc 的存在让 IME 透明性成立(选候选/上屏/取消组词,
// 见 specs/virtual-keyboard/spec.md 的 IME 透明性需求)。
KeyboardLayout defaultKeyboardLayout();

// 键盘导航纯逻辑(design.md D9:不接触任何 QWidget,可离屏单测):
// - 摇杆/十字键量化出的方向驱动高亮移动,持续按住由重复定时器接力;
// - A 确认高亮键位 → 经 KeyInjector 注入;B 独立退格;
// - sticky Shift(design.md D7):点亮后下一个字母以 Shift+字母 组合
//   注入并自动熄灭,绝不单独发送 Shift。
class KeyboardNavController : public QObject {
    Q_OBJECT
  public:
    explicit KeyboardNavController(KeyInjector& injector, QObject* parent = nullptr);

    // 摇杆/十字键的当前量化方向。方向变化立即走一格并重启重复定时器;
    // 持续同方向交给定时器按间隔重复;(0,0) 停止。
    void setDirection(int dx, int dy);
    void confirmKey();   // A:确认高亮键位
    void backspaceKey(); // B:独立退格,等价确认退格键
    void resetState();   // 高亮复位 + Shift 熄灭 + 停止重复(关闭键盘时调用)

    void setRepeatTimings(int delayMs, int intervalMs);

    int row() const { return m_row; }
    int col() const { return m_col; }
    bool shiftLatched() const { return m_shiftLatched; }
    const KeyboardLayout& layout() const { return m_layout; }

  signals:
    void stateChanged();   // 高亮/Shift 变化 → overlay 重绘
    void closeRequested(); // 确认了"关闭"键位

  private:
    void stepBy(int dx, int dy);
    static int centerXUnits(const KbRow& row, int col);
    void onRepeatTimeout();

    KeyInjector& m_injector;
    KeyboardLayout m_layout;
    int m_row = 0;
    int m_col = 0;
    bool m_shiftLatched = false;
    int m_dx = 0;
    int m_dy = 0;
    QTimer m_repeatTimer;
    bool m_inRepeatPhase = false;
    int m_delayMs = 400;
    int m_intervalMs = 150;
};
