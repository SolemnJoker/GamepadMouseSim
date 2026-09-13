#include "HelpContent.h"
#include "core/Config.h"
#include "core/MappingDefaults.h"
#include "core/Types.h"

namespace {

QVector<HelpLine> mappingSection(const QMap<QString, ButtonAction>& mapping,
                                 const QStringList& order) {
    QVector<HelpLine> lines;
    for (const QString& btn : order) {
        const ButtonAction action = mapping.value(btn, ButtonAction::None);
        if (action == ButtonAction::None)
            continue; // 未绑定行不展示
        lines.push_back({btn, actionToChinese(action)});
    }
    return lines;
}

} // namespace

namespace HelpContent {

QVector<HelpSection> build(const Config& config) {
    const auto direct = MappingDefaults::mergedDirect(
        config.value("mouse_mode.button_mapping").toJsonValue().toObject());
    const QJsonObject modObj = config.value("mouse_mode.modifier_mapping").toJsonValue().toObject();
    const auto lt = MappingDefaults::mergedLayer("LT", modObj);
    const auto rt = MappingDefaults::mergedLayer("RT", modObj);

    const QStringList buttonOrder = {"A",        "B",         "X",    "Y",      "LB",
                                     "RB",       "View",      "Menu", "DpadUp", "DpadDown",
                                     "DpadLeft", "DpadRight", "L3",   "R3"};

    QVector<HelpSection> sections;
    sections.push_back({QStringLiteral("直接映射"),
                        mappingSection(direct, QStringList{"A", "B", "X", "Y", "LB", "RB"})});
    sections.push_back(
        {QStringLiteral("方向键(直接)"),
         mappingSection(direct, QStringList{"DpadUp", "DpadDown", "DpadLeft", "DpadRight"})});
    sections.push_back({QStringLiteral("L3层 (按住L3)"), mappingSection(lt, buttonOrder)});
    sections.push_back({QStringLiteral("RT层 (按住RT)"), mappingSection(rt, buttonOrder)});

    // 摇杆区:参数来自配置,文案固定。
    QVector<HelpLine> stick;
    stick.push_back({QStringLiteral("左摇杆"), QStringLiteral("移动鼠标")});
    stick.push_back({QStringLiteral("右摇杆"), QStringLiteral("滚动页面")});
    sections.push_back({QStringLiteral("摇杆"), stick});

    // 模式与系统区:全部动态抓取,保证与生效绑定一致。
    QVector<HelpLine> system;
    QVariant comboV = config.value("combo_key.buttons");
    QStringList comboBtns;
    if (comboV.canConvert<QVariantList>()) {
        for (const QVariant& item : comboV.toList())
            comboBtns << item.toString();
    }
    if (!comboBtns.isEmpty())
        system.push_back(
            {comboBtns.join("+") + QStringLiteral("(长按1秒)"), QStringLiteral("切换模式")});
    for (auto it = lt.begin(); it != lt.end(); ++it) {
        if (it.value() == ButtonAction::ShowHelp)
            system.push_back({QStringLiteral("L3+") + it.key(), actionToChinese(it.value())});
        if (it.value() == ButtonAction::ShowKeyboard)
            system.push_back({QStringLiteral("L3+") + it.key(), actionToChinese(it.value())});
    }
    sections.push_back({QStringLiteral("模式与系统"), system});

    return sections;
}

} // namespace HelpContent
