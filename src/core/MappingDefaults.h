#pragma once

#include "core/Types.h"
#include <QJsonObject>
#include <QMap>
#include <QString>

// 按键映射的内置默认值——**唯一事实源**(design.md D2)。
// 运行时兜底(KeyboardMapper)、设置 GUI 显示/保存、帮助屏内容、
// config 迁移补默认全部从这里读取;config/default_config.json 是
// 同一数据的 JSON 投影(test_mapping_defaults 校验两者一致)。
namespace MappingDefaults {

QMap<QString, ButtonAction> directDefaults();
QMap<QString, ButtonAction> ltLayerDefaults();
QMap<QString, ButtonAction> rtLayerDefaults();

// 默认表的 JSON 形态(设置 GUI"恢复默认"直接写入配置用)。
QJsonObject directDefaultsJson();
QJsonObject ltLayerDefaultsJson();
QJsonObject rtLayerDefaultsJson();

// 默认表 ⊕ 配置覆盖:配置里**存在**的键生效(显式 "None" 按用户意图
// 生效为无),缺失的键保留默认(design.md D8 语义)。
QMap<QString, ButtonAction> mergedDirect(const QJsonObject& configMapping);
QMap<QString, ButtonAction> mergedLayer(const QString& layerName, const QJsonObject& configLayers);

// 完整 mouse_mode 方案骨架 ⊕ 覆盖:用于 profile 展开/迁移时保证
// mouse_mode 各子节完整(profile 缺子键时补默认)。
QJsonObject skeletonMouseMode();
QJsonObject mergedMouseMode(const QJsonObject& mouseMode);

// 通用深合并:以 defaults 为骨架,user 中存在的键胜出(对象递归,
// 数组/标量整体覆盖)。迁移补默认与 mouse_mode 合并共用。
QJsonObject mergedDeep(const QJsonObject& defaults, const QJsonObject& user);

} // namespace MappingDefaults
