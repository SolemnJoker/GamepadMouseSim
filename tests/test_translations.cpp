#include "test_translations.h"
#include "core/Translations.h"
#include "core/Types.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSet>

void TestTranslations::fallback_unknownKeyReturnsSource() {
    QCOMPARE(
        Translations::translate(QStringLiteral("不存在的条目"), Translations::Language::English),
        QStringLiteral("不存在的条目"));
    QCOMPARE(Translations::translate(QStringLiteral("设置"), Translations::Language::Chinese),
             QStringLiteral("设置"));
}

void TestTranslations::resolve_threeStates() {
    QCOMPARE(Translations::resolveUiLanguage(QStringLiteral("zh")),
             Translations::Language::Chinese);
    QCOMPARE(Translations::resolveUiLanguage(QStringLiteral("en")),
             Translations::Language::English);
    // "system" 依赖本机 locale:断言结果是合法语言,未知取值与 system 同路
    const auto sys = Translations::resolveUiLanguage(QStringLiteral("system"));
    QVERIFY(sys == Translations::Language::Chinese || sys == Translations::Language::English);
    QCOMPARE(Translations::resolveUiLanguage(QStringLiteral("fr")), sys);
}

void TestTranslations::setLanguage_changesTrOutput() {
    Translations::setLanguage(Translations::Language::Chinese);
    QCOMPARE(Translations::tr(QStringLiteral("设置")), QStringLiteral("设置"));
    Translations::setLanguage(Translations::Language::English);
    QCOMPARE(Translations::tr(QStringLiteral("设置")), QStringLiteral("Settings"));
    Translations::setLanguage(Translations::Language::Chinese); // 恢复默认(其余测试依赖)
}

void TestTranslations::actionNames_bilingual() {
    Translations::setLanguage(Translations::Language::Chinese);
    QCOMPARE(actionToChinese(ButtonAction::MouseLeftClick), QStringLiteral("左键单击"));
    QCOMPARE(actionToChinese(ButtonAction::ShowKeyboard), QStringLiteral("虚拟键盘"));

    Translations::setLanguage(Translations::Language::English);
    QCOMPARE(actionToChinese(ButtonAction::MouseLeftClick), QStringLiteral("Left click"));
    QCOMPARE(actionToChinese(ButtonAction::ShowKeyboard), QStringLiteral("On-screen keyboard"));
    Translations::setLanguage(Translations::Language::Chinese);
}

// 全量源码扫描(design D4):逐文件提取 Translations::tr("...") 的字面参数,
// 断言每条都在英文表内。相邻字符串字面量拼接(多行文案)先做归一化。
void TestTranslations::sourceScan_everyTrLiteralHasEnglishEntry() {
    const QString srcRoot = QStringLiteral(PROJECT_SOURCE_DIR "/src");
    QVERIFY2(QDir(srcRoot).exists(), qPrintable("source dir not found: " + srcRoot));

    // 显式豁免(登记在此并注释原因);当前为空。
    const QSet<QString> exemptions = {};

    // 常规转义字面量(AutoMoc 对 raw string 过敏):
    //   Translations::tr\(\s*"((?:[^"\\]|\\.)*)"
    static const QRegularExpression trRe(
        QStringLiteral("Translations::tr\\(\\s*\"((?:[^\"\\\\]|\\\\.)*)\""));
    int scanned = 0;
    QStringList missing;

    QDirIterator it(srcRoot, {"*.cpp", "*.h"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QString content = QString::fromUtf8(f.readAll());

        // 归一化 C++ 相邻字面量拼接:  "..." 换行 "..."  -> 拼为一段
        content.replace(QRegularExpression(QStringLiteral("\"\\s*\\n\\s*\"")), QString());

        auto matches = trRe.globalMatch(content);
        while (matches.hasNext()) {
            QString key = matches.next().captured(1);
            // 源码字面量里的 C++ 转义(")解码为运行时字符串,再与表比对
            key.replace(QStringLiteral("\\\""), QStringLiteral("\""));
            ++scanned;
            if (exemptions.contains(key))
                continue;
            if (!Translations::englishTableContains(key))
                missing << QStringLiteral("%1: %2").arg(QDir(srcRoot).relativeFilePath(path), key);
        }
    }

    QVERIFY2(scanned > 100,
             qPrintable(QStringLiteral("scan found only %1 tr() literals").arg(scanned)));
    if (!missing.isEmpty())
        qWarning().noquote() << "missing English entries:\n" << missing.join('\n');
    QCOMPARE(missing.size(), 0);
}

QTEST_MAIN(TestTranslations)
#include "test_translations.moc"
