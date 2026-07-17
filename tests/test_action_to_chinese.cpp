#include "test_action_to_chinese.h"
#include <QDebug>

void TestActionToChinese::allActions_haveNonEmptyMapping() {
    // Iterate over all ButtonAction enum values from first (None) to last (ShowHelp).
    constexpr int first = static_cast<int>(ButtonAction::None);
    constexpr int last  = static_cast<int>(ButtonAction::ShowHelp);
    int failures = 0;

    for (int i = first; i <= last; ++i) {
        auto action = static_cast<ButtonAction>(i);
        if (action == ButtonAction::None)
            continue; // None maps to "None" / "无" — not a real action

        const QString label = actionToChinese(action);
        if (label.isEmpty()) {
            qWarning() << "actionToChinese(" << i << ") returned empty string";
            ++failures;
        } else if (label == "无") {
            // "无" means the default case was hit — the enum value has no mapping.
            qWarning() << "actionToChinese("
                       << actionToString(action)
                       << ") fell through to default: return \"无\"";
            ++failures;
        }
    }

    QCOMPARE(failures, 0);
}