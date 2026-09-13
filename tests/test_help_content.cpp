#include "test_help_content.h"
#include "core/Config.h"
#include "ui/HelpContent.h"
#include <QTemporaryDir>

namespace {
QVector<HelpLine> flattenLines(const QVector<HelpSection>& sections) {
    QVector<HelpLine> out;
    for (const HelpSection& s : sections)
        for (const HelpLine& l : s.lines)
            out.push_back(l);
    return out;
}
} // namespace

void TestHelpContent::init() {
    m_dir = new QTemporaryDir();
    QVERIFY(m_dir->isValid());
    m_config = new Config(this);
    QVERIFY(m_config->load(m_dir->filePath("config.json")));
}

void TestHelpContent::cleanup() {
    delete m_config;
    m_config = nullptr;
    delete m_dir;
    m_dir = nullptr;
}

void TestHelpContent::defaultContent_matchesDefaults() {
    const auto sections = HelpContent::build(*m_config);
    QVERIFY(sections.size() >= 5); // 直接/方向/L3/RT/摇杆/模式与系统

    // 分区标题齐全
    QStringList titles;
    for (const auto& s : sections)
        titles << s.title;
    QVERIFY(titles.contains(QStringLiteral("直接映射")));
    QVERIFY(titles.contains(QStringLiteral("L3层 (按住L3)")));
    QVERIFY(titles.contains(QStringLiteral("RT层 (按住RT)")));
    QVERIFY(titles.contains(QStringLiteral("摇杆")));
    QVERIFY(titles.contains(QStringLiteral("模式与系统")));

    // 默认绑定出现在内容里
    const auto lines = flattenLines(sections);
    bool hasA = false;
    for (const auto& l : lines)
        if (l.button == "A" && l.action == QStringLiteral("左键单击"))
            hasA = true;
    QVERIFY2(hasA, "default A=左键单击 must appear in help content");
}

void TestHelpContent::content_followsConfigChange() {
    m_config->setValue("mouse_mode.button_mapping.A", QStringLiteral("MouseRightClick"));
    const auto sections = HelpContent::build(*m_config);
    const auto lines = flattenLines(sections);
    bool hasNew = false;
    for (const auto& l : lines)
        if (l.button == "A" && l.action == QStringLiteral("右键单击"))
            hasNew = true;
    QVERIFY2(hasNew, "help content must reflect config change immediately");
}

void TestHelpContent::systemSection_followsDynamicBindings() {
    // 默认:LT 层 Menu=ShowKeyboard → 系统区含 "L3+Menu 虚拟键盘"
    auto sections = HelpContent::build(*m_config);
    bool hasKeyboard = false;
    for (const auto& s : sections) {
        if (s.title != QStringLiteral("模式与系统"))
            continue;
        for (const auto& l : s.lines)
            if (l.button == QStringLiteral("L3+Menu") && l.action == QStringLiteral("虚拟键盘"))
                hasKeyboard = true;
    }
    QVERIFY2(hasKeyboard, "default keyboard binding must appear in system section");

    // 用户改绑为 None → 该行消失
    m_config->setValue("mouse_mode.modifier_mapping.LT.Menu", QStringLiteral("None"));
    sections = HelpContent::build(*m_config);
    hasKeyboard = false;
    for (const auto& s : sections) {
        if (s.title != QStringLiteral("模式与系统"))
            continue;
        for (const auto& l : s.lines)
            if (l.button == QStringLiteral("L3+Menu"))
                hasKeyboard = true;
    }
    QVERIFY2(!hasKeyboard, "unbound keyboard entry must disappear from system section");
}

QTEST_MAIN(TestHelpContent)
#include "test_help_content.moc"
