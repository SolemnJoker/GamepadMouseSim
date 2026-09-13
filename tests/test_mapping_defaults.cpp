#include "test_mapping_defaults.h"
#include "core/MappingDefaults.h"
#include "core/Types.h"
#include <QFile>
#include <QJsonDocument>

void TestMappingDefaults::allButtons_haveDefaultsInAllThreeLayers() {
    const QStringList buttons = {"A",        "B",         "X",    "Y",   "DpadUp", "DpadDown",
                                 "DpadLeft", "DpadRight", "LB",   "RB",  "LT",     "RT",
                                 "L3",       "R3",        "View", "Menu"};
    const QMap<QString, ButtonAction> direct = MappingDefaults::directDefaults();
    const QMap<QString, ButtonAction> lt = MappingDefaults::ltLayerDefaults();
    const QMap<QString, ButtonAction> rt = MappingDefaults::rtLayerDefaults();

    int failures = 0;
    for (const QString& b : buttons) {
        if (!direct.contains(b) || !lt.contains(b) || !rt.contains(b)) {
            qWarning() << "missing default for button" << b;
            ++failures;
        }
    }
    QCOMPARE(failures, 0);
}

void TestMappingDefaults::mergedDirect_missingKeysFallBack_explicitWins() {
    // 空 config → 全默认(含 LT 层 Menu=ShowKeyboard 这类新绑定)
    QJsonObject empty;
    QCOMPARE(MappingDefaults::mergedDirect(empty).value("A"), ButtonAction::MouseLeftClick);

    // 显式值胜出
    QJsonObject custom{{"A", "MouseRightClick"}};
    QCOMPARE(MappingDefaults::mergedDirect(custom).value("A"), ButtonAction::MouseRightClick);
    // 未覆盖的键仍为默认
    QCOMPARE(MappingDefaults::mergedDirect(custom).value("B"), ButtonAction::MouseRightClick);

    // 显式 None 胜出(用户意图生效为无)
    QJsonObject none{{"Y", "None"}};
    QCOMPARE(MappingDefaults::mergedDirect(none).value("Y"), ButtonAction::None);
}

void TestMappingDefaults::mergedLayer_semantics() {
    QJsonObject layers;
    QCOMPARE(MappingDefaults::mergedLayer("LT", layers).value("Menu"),
             ButtonAction::ShowKeyboard); // 旧配置缺 Menu → 默认补上
    QCOMPARE(MappingDefaults::mergedLayer("RT", layers).value("A"), ButtonAction::KeyCtrlA);

    QJsonObject customLayers;
    customLayers["LT"] = QJsonObject{{"Menu", "None"}};
    QCOMPARE(MappingDefaults::mergedLayer("LT", customLayers).value("Menu"), ButtonAction::None);
    QCOMPARE(MappingDefaults::mergedLayer("LT", customLayers).value("X"), ButtonAction::KeyAltTab);

    // 未知层名(历史遗留):空表起步,仅含显式键
    QJsonObject legacy;
    legacy["L3"] = QJsonObject{{"A", "Enter"}};
    QCOMPARE(MappingDefaults::mergedLayer("L3", legacy).value("A"), ButtonAction::KeyEnter);
    QVERIFY(!MappingDefaults::mergedLayer("L3", legacy).contains("X"));
}

// default_config.json 是默认表的 JSON 投影:存在的键必须与默认表一致,
// 不允许出现默认表之外的键(缺失键允许——投影可以只写子集)。
void TestMappingDefaults::defaultConfigJson_matchesDefaults() {
    QFile res(QStringLiteral(":/config/default_config.json"));
    QVERIFY2(res.open(QIODevice::ReadOnly), "builtin default config resource must be readable");
    const QJsonObject root = QJsonDocument::fromJson(res.readAll()).object();
    res.close();
    QCOMPARE(root.value("schema_version").toInt(), kCurrentConfigSchemaVersion);

    const QJsonObject mouseMode = root.value("mouse_mode").toObject();
    const auto checkLayer = [](const QJsonObject& jsonLayer,
                               const QMap<QString, ButtonAction>& defaults) {
        int failures = 0;
        for (auto it = jsonLayer.begin(); it != jsonLayer.end(); ++it) {
            if (!defaults.contains(it.key())) {
                qWarning() << "json key not in defaults:" << it.key();
                ++failures;
                continue;
            }
            if (stringToAction(it.value().toString()) != defaults.value(it.key())) {
                qWarning() << "json value mismatch for" << it.key() << "=" << it.value().toString();
                ++failures;
            }
        }
        return failures;
    };

    int failures =
        checkLayer(mouseMode.value("button_mapping").toObject(), MappingDefaults::directDefaults());
    const QJsonObject mods = mouseMode.value("modifier_mapping").toObject();
    failures += checkLayer(mods.value("LT").toObject(), MappingDefaults::ltLayerDefaults());
    failures += checkLayer(mods.value("RT").toObject(), MappingDefaults::rtLayerDefaults());
    QCOMPARE(failures, 0);
}

QTEST_MAIN(TestMappingDefaults)
#include "test_mapping_defaults.moc"
