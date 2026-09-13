#pragma once

#include <QtTest>

// 翻译模块(design D4):回退、语言解析、热切换;
// **全量源码扫描**:src/ 下所有 Translations::tr("...") 字面量必须
// 在英文表内(中文原文 key 模式的漂移防护——改原文即测试红)。
class TestTranslations : public QObject {
    Q_OBJECT
  private slots:
    void fallback_unknownKeyReturnsSource();
    void resolve_threeStates();
    void setLanguage_changesTrOutput();
    void actionNames_bilingual();
    void sourceScan_everyTrLiteralHasEnglishEntry();
};
