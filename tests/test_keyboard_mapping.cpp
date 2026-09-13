#include "test_keyboard_mapping.h"
#include "core/Config.h"
#include "core/Types.h"
#include "input/KeyboardMapper.h"
#include <QSignalSpy>
#include <QTemporaryFile>

void TestKeyboardMapping::init() {
    m_tmpFile = new QTemporaryFile(this);
    m_config = nullptr;
    m_mapper = nullptr;
}

void TestKeyboardMapping::cleanup() {
    delete m_mapper;
    m_mapper = nullptr;
    delete m_config;
    m_config = nullptr;
    delete m_tmpFile;
    m_tmpFile = nullptr;
}

void TestKeyboardMapping::writeConfig(const QByteArray& json) {
    QVERIFY(m_tmpFile->open());
    m_tmpFile->write(json);
    m_tmpFile->close();

    m_config = new Config(this);
    QVERIFY(m_config->load(m_tmpFile->fileName()));
    m_mapper = new KeyboardMapper(m_config, this);
}

void TestKeyboardMapping::ltLayerMissingMenu_fallsBackToShowKeyboard() {
    // LT 层存在但没有 Menu 键 → Menu 保留内置默认 ShowKeyboard(D8)
    writeConfig(
        QByteArrayLiteral("{\"mouse_mode\":{\"modifier_mapping\":{\"LT\":{\"X\":\"Alt+Tab\"}}}}"));
    QSignalSpy spy(m_mapper, &KeyboardMapper::showKeyboardRequested);
    m_mapper->processButton(XINPUT_GAMEPAD_LEFT_THUMB, true, 0);
    m_mapper->processButton(XINPUT_GAMEPAD_START, true, 0);
    QCOMPARE(spy.count(), 1);
}

void TestKeyboardMapping::noModifierLayers_fallsBackToShowKeyboard() {
    writeConfig(QByteArrayLiteral("{}"));
    QSignalSpy spy(m_mapper, &KeyboardMapper::showKeyboardRequested);
    m_mapper->processButton(XINPUT_GAMEPAD_LEFT_THUMB, true, 0);
    m_mapper->processButton(XINPUT_GAMEPAD_START, true, 0);
    QCOMPARE(spy.count(), 1);
}

void TestKeyboardMapping::explicitNone_inConfigOverridesDefault() {
    // 显式 "None" 是存在的键:按用户意图生效为无,不触发
    writeConfig(
        QByteArrayLiteral("{\"mouse_mode\":{\"modifier_mapping\":{\"LT\":{\"Menu\":\"None\"}}}}"));
    QSignalSpy spy(m_mapper, &KeyboardMapper::showKeyboardRequested);
    m_mapper->processButton(XINPUT_GAMEPAD_LEFT_THUMB, true, 0);
    m_mapper->processButton(XINPUT_GAMEPAD_START, true, 0);
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(TestKeyboardMapping)
#include "test_keyboard_mapping.moc"
