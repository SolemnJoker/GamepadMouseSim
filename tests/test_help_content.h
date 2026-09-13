#pragma once

#include <QtTest>

class Config;

// 帮助内容数据层(design.md D5 L1):内容与合并视图一致、随配置变化。
class TestHelpContent : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void defaultContent_matchesDefaults();
    void content_followsConfigChange();
    void systemSection_followsDynamicBindings();

  private:
    QTemporaryDir* m_dir = nullptr;
    Config* m_config = nullptr;
};
