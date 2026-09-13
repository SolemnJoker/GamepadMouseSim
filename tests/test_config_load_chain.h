#pragma once

#include <QTemporaryDir>
#include <QtTest>

class Config;

// 配置加载链(design D1)+ 增量迁移(design D4)。
class TestConfigLoadChain : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void bareExe_firstLaunch_generatesWritableConfig();
    void legacyV1Config_incrementalMerge_preservesCustom();
    void corruptJson_resetsToFactory_andRewritesValidFile();
    void restoreFactoryDefaults_replacesUserKeys_andRewritesFile();
    void existingV2Config_loadDoesNotTouchMouseMode();

  private:
    void writeRaw(const QByteArray& json);

    QTemporaryDir* m_dir = nullptr;
    Config* m_config = nullptr;
};
