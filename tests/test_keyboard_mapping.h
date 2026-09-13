#pragma once

#include <QtTest>

class Config;
class QTemporaryFile;
class KeyboardMapper;

// D8 默认合并(design.md):旧配置缺失的映射键回退到代码内置默认表,
// 保证升级用户零操作获得新绑定(LT 层 Menu → ShowKeyboard)。
class TestKeyboardMapping : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void ltLayerMissingMenu_fallsBackToShowKeyboard();
    void noModifierLayers_fallsBackToShowKeyboard();
    void explicitNone_inConfigOverridesDefault();

  private:
    void writeConfig(const QByteArray& json);

    QTemporaryFile* m_tmpFile = nullptr;
    Config* m_config = nullptr;
    KeyboardMapper* m_mapper = nullptr;
};
