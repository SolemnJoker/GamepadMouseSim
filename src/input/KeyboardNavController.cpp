#include "KeyboardNavController.h"

int KeyboardLayout::maxRowWidthUnits() const {
    int maxUnits = 0;
    for (const KbRow& row : rows) {
        int units = 0;
        for (const KbKey& key : row.keys)
            units += key.width;
        maxUnits = qMax(maxUnits, units);
    }
    return maxUnits;
}

static KbKey charKey(const char* label, WORD vk) {
    return KbKey{QString::fromLatin1(label), vk, KbKey::Kind::Char, 1};
}

KeyboardLayout defaultKeyboardLayout() {
    KeyboardLayout layout;
    // 数字行 + 退格(IME:数字选候选)
    layout.rows.push_back(
        KbRow{{charKey("1", '1'), charKey("2", '2'), charKey("3", '3'), charKey("4", '4'),
               charKey("5", '5'), charKey("6", '6'), charKey("7", '7'), charKey("8", '8'),
               charKey("9", '9'), charKey("0", '0'),
               KbKey{QStringLiteral("\u232B"), VK_BACK, KbKey::Kind::Backspace, 2}}});
    // QWERTY 上排 + Esc(IME:取消组词)
    layout.rows.push_back(
        KbRow{{charKey("Q", 'Q'), charKey("W", 'W'), charKey("E", 'E'), charKey("R", 'R'),
               charKey("T", 'T'), charKey("Y", 'Y'), charKey("U", 'U'), charKey("I", 'I'),
               charKey("O", 'O'), charKey("P", 'P'),
               KbKey{QStringLiteral("Esc"), VK_ESCAPE, KbKey::Kind::Escape, 1}}});
    // QWERTY 中排 + 回车(IME:字母上屏)
    layout.rows.push_back(KbRow{
        {charKey("A", 'A'), charKey("S", 'S'), charKey("D", 'D'), charKey("F", 'F'),
         charKey("G", 'G'), charKey("H", 'H'), charKey("J", 'J'), charKey("K", 'K'),
         charKey("L", 'L'), KbKey{QStringLiteral("\u21B5"), VK_RETURN, KbKey::Kind::Enter, 2}}});
    // QWERTY 下排 + Shift(sticky)+ 关闭
    layout.rows.push_back(
        KbRow{{KbKey{QStringLiteral("\u21E7"), VK_SHIFT, KbKey::Kind::Shift, 2}, charKey("Z", 'Z'),
               charKey("X", 'X'), charKey("C", 'C'), charKey("V", 'V'), charKey("B", 'B'),
               charKey("N", 'N'), charKey("M", 'M'),
               KbKey{QStringLiteral("\u2715"), 0, KbKey::Kind::Close, 2}}});
    // 空格(IME:首选候选上屏)
    layout.rows.push_back(KbRow{{KbKey{QStringLiteral("Space"), VK_SPACE, KbKey::Kind::Space, 8}}});
    return layout;
}

KeyboardNavController::KeyboardNavController(KeyInjector& injector, QObject* parent)
    : QObject(parent), m_injector(injector), m_layout(defaultKeyboardLayout()) {
    m_repeatTimer.setSingleShot(true);
    connect(&m_repeatTimer, &QTimer::timeout, this, &KeyboardNavController::onRepeatTimeout);
}

void KeyboardNavController::setRepeatTimings(int delayMs, int intervalMs) {
    m_delayMs = qMax(0, delayMs);
    m_intervalMs = qMax(0, intervalMs);
}

void KeyboardNavController::setDirection(int dx, int dy) {
    if (dx == m_dx && dy == m_dy)
        return; // 持续同方向:交给重复定时器
    m_dx = dx;
    m_dy = dy;
    m_repeatTimer.stop();
    m_inRepeatPhase = false;
    if (dx == 0 && dy == 0)
        return;
    stepBy(dx, dy);
    if (m_delayMs >= 0)
        m_repeatTimer.start(m_delayMs);
}

void KeyboardNavController::confirmKey() {
    const KbKey& key = m_layout.rows[m_row].keys[m_col];
    switch (key.kind) {
    case KbKey::Kind::Shift:
        m_shiftLatched = !m_shiftLatched;
        emit stateChanged();
        break;
    case KbKey::Kind::Close:
        emit closeRequested();
        break;
    case KbKey::Kind::Space:
        m_injector.sendVk(VK_SPACE, false);
        break;
    case KbKey::Kind::Backspace:
        m_injector.sendVk(VK_BACK, false);
        break;
    case KbKey::Kind::Enter:
        m_injector.sendVk(VK_RETURN, false);
        break;
    case KbKey::Kind::Escape:
        m_injector.sendVk(VK_ESCAPE, false);
        break;
    case KbKey::Kind::Char: {
        const bool isLetter = key.vk >= 'A' && key.vk <= 'Z';
        const bool withShift = m_shiftLatched && isLetter;
        m_injector.sendVk(key.vk, withShift);
        if (withShift) {
            m_shiftLatched = false;
            emit stateChanged();
        }
        break;
    }
    }
}

void KeyboardNavController::backspaceKey() {
    m_injector.sendVk(VK_BACK, false);
}

void KeyboardNavController::resetState() {
    m_dx = 0;
    m_dy = 0;
    m_repeatTimer.stop();
    m_inRepeatPhase = false;
    m_row = 0;
    m_col = 0;
    m_shiftLatched = false;
    emit stateChanged();
}

int KeyboardNavController::centerXUnits(const KbRow& row, int col) {
    int units = 0;
    for (int i = 0; i < col && i < static_cast<int>(row.keys.size()); ++i)
        units += row.keys[i].width;
    return units + row.keys[col].width / 2;
}

void KeyboardNavController::stepBy(int dx, int dy) {
    const int rows = static_cast<int>(m_layout.rows.size());
    int newRow = qBound(0, m_row + dy, rows - 1);
    int newCol = m_col;

    if (dx != 0) {
        const KbRow& row = m_layout.rows[newRow];
        newCol = qBound(0, m_col + dx, static_cast<int>(row.keys.size()) - 1);
    } else if (dy != 0) {
        // 垂直移动按水平中心点就近对齐(行宽不等时保持视觉列感)。
        const int target = centerXUnits(m_layout.rows[m_row], m_col);
        const KbRow& row = m_layout.rows[newRow];
        int best = 0;
        int bestDist = -1;
        for (int i = 0; i < static_cast<int>(row.keys.size()); ++i) {
            const int dist = qAbs(centerXUnits(row, i) - target);
            if (bestDist < 0 || dist < bestDist) {
                bestDist = dist;
                best = i;
            }
        }
        newCol = best;
    }

    if (newRow != m_row || newCol != m_col) {
        m_row = newRow;
        m_col = newCol;
        emit stateChanged();
    }
}

void KeyboardNavController::onRepeatTimeout() {
    if (m_dx == 0 && m_dy == 0)
        return;
    m_inRepeatPhase = true;
    stepBy(m_dx, m_dy);
    m_repeatTimer.start(m_intervalMs);
}
