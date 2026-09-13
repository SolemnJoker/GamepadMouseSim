#include "MappingDefaults.h"

namespace MappingDefaults {

namespace {

QJsonObject mappingToJson(const QMap<QString, ButtonAction>& map) {
    QJsonObject obj;
    for (auto it = map.begin(); it != map.end(); ++it)
        obj[it.key()] = actionToString(it.value());
    return obj;
}

QMap<QString, ButtonAction> mergedLayerObject(const QMap<QString, ButtonAction>& defaults,
                                              const QJsonObject& configLayer) {
    QMap<QString, ButtonAction> merged = defaults;
    for (auto it = configLayer.begin(); it != configLayer.end(); ++it)
        merged[it.key()] = stringToAction(it.value().toString());
    return merged;
}

} // namespace

QMap<QString, ButtonAction> directDefaults() {
    return {{"A", ButtonAction::MouseLeftClick},
            {"B", ButtonAction::MouseRightClick},
            {"X", ButtonAction::MouseMiddleClick},
            {"Y", ButtonAction::KeyEnter},
            {"DpadUp", ButtonAction::KeyArrowUp},
            {"DpadDown", ButtonAction::KeyArrowDown},
            {"DpadLeft", ButtonAction::KeyArrowLeft},
            {"DpadRight", ButtonAction::KeyArrowRight},
            {"LB", ButtonAction::MouseLeftHold},
            {"RB", ButtonAction::MouseRightHold},
            {"LT", ButtonAction::None},
            {"RT", ButtonAction::None},
            {"L3", ButtonAction::None},
            {"R3", ButtonAction::KeyEscape},
            {"View", ButtonAction::KeyTab},
            {"Menu", ButtonAction::KeyWin}};
}

QMap<QString, ButtonAction> ltLayerDefaults() {
    return {{"X", ButtonAction::KeyAltTab},
            {"LB", ButtonAction::KeyShiftTab},
            {"RB", ButtonAction::KeyTab},
            {"A", ButtonAction::KeyEnter},
            {"B", ButtonAction::KeyEscape},
            {"Y", ButtonAction::KeyAltF4},
            {"DpadUp", ButtonAction::VolumeUp},
            {"DpadDown", ButtonAction::VolumeDown},
            {"DpadLeft", ButtonAction::VolumeMute},
            {"DpadRight", ButtonAction::None},
            {"R3", ButtonAction::ShowHelp},
            {"View", ButtonAction::None},
            {"Menu", ButtonAction::ShowKeyboard},
            {"L3", ButtonAction::None},
            {"RT", ButtonAction::None},
            {"LT", ButtonAction::None}};
}

QMap<QString, ButtonAction> rtLayerDefaults() {
    return {{"A", ButtonAction::KeyCtrlA},
            {"B", ButtonAction::KeyCtrlC},
            {"X", ButtonAction::KeyCtrlX},
            {"Y", ButtonAction::KeyCtrlV},
            {"DpadUp", ButtonAction::KeyPageUp},
            {"DpadDown", ButtonAction::KeyPageDown},
            {"DpadLeft", ButtonAction::KeyCtrlShiftTab},
            {"DpadRight", ButtonAction::None},
            {"LB", ButtonAction::None},
            {"RB", ButtonAction::KeyCtrlS},
            {"L3", ButtonAction::KeyWinD},
            {"R3", ButtonAction::KeyCtrlZ},
            {"View", ButtonAction::None},
            {"Menu", ButtonAction::None},
            {"LT", ButtonAction::None},
            {"RT", ButtonAction::None}};
}

QJsonObject directDefaultsJson() {
    return mappingToJson(directDefaults());
}

QJsonObject ltLayerDefaultsJson() {
    return mappingToJson(ltLayerDefaults());
}

QJsonObject rtLayerDefaultsJson() {
    return mappingToJson(rtLayerDefaults());
}

QMap<QString, ButtonAction> mergedDirect(const QJsonObject& configMapping) {
    QMap<QString, ButtonAction> merged = directDefaults();
    for (auto it = configMapping.begin(); it != configMapping.end(); ++it)
        merged[it.key()] = stringToAction(it.value().toString());
    return merged;
}

QMap<QString, ButtonAction> mergedLayer(const QString& layerName, const QJsonObject& configLayers) {
    const QJsonObject configLayer = configLayers.value(layerName).toObject();
    if (layerName == QLatin1String("LT"))
        return mergedLayerObject(ltLayerDefaults(), configLayer);
    if (layerName == QLatin1String("RT"))
        return mergedLayerObject(rtLayerDefaults(), configLayer);
    // 未知层名(如历史遗留 "L3"):空表起步,仅含配置显式给出的键。
    QMap<QString, ButtonAction> merged;
    for (auto it = configLayer.begin(); it != configLayer.end(); ++it)
        merged[it.key()] = stringToAction(it.value().toString());
    return merged;
}

QJsonObject skeletonMouseMode() {
    QJsonObject leftStick{
        {"sensitivity_x", 1.0}, {"sensitivity_y", 1.0}, {"deadzone", 0.15}, {"acceleration", true}};
    QJsonObject rightStick{
        {"scroll_speed_vertical", 1.0}, {"scroll_speed_horizontal", 1.0}, {"deadzone", 0.15}};
    QJsonObject modifierMapping;
    modifierMapping["LT"] = mappingToJson(ltLayerDefaults());
    modifierMapping["RT"] = mappingToJson(rtLayerDefaults());
    QJsonObject obj;
    obj["left_stick"] = leftStick;
    obj["right_stick"] = rightStick;
    obj["button_mapping"] = mappingToJson(directDefaults());
    obj["modifier_mapping"] = modifierMapping;
    return obj;
}

QJsonObject mergedDeep(const QJsonObject& defaults, const QJsonObject& user) {
    QJsonObject out = defaults;
    for (auto it = user.begin(); it != user.end(); ++it) {
        const QJsonObject userChild = it.value().toObject();
        if (it.value().isObject() && out.value(it.key()).isObject()) {
            out[it.key()] = mergedDeep(out.value(it.key()).toObject(), userChild);
        } else {
            out[it.key()] = it.value();
        }
    }
    return out;
}

QJsonObject mergedMouseMode(const QJsonObject& mouseMode) {
    return mergedDeep(skeletonMouseMode(), mouseMode);
}

} // namespace MappingDefaults
