#pragma once

#include <QtTest>

// MappingDefaults:默认映射的唯一事实源(design D2)。
class TestMappingDefaults : public QObject {
    Q_OBJECT
  private slots:
    void allButtons_haveDefaultsInAllThreeLayers();
    void mergedDirect_missingKeysFallBack_explicitWins();
    void mergedLayer_semantics();
    void defaultConfigJson_matchesDefaults();
};
