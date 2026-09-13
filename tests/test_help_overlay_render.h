#pragma once

#include <QtTest>

class Config;
class OsdOverlay;

// 帮助屏离屏渲染(design.md D7 L3):内容渲染非空、配置变化后画面变化。
class TestHelpOverlayRender : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void grab_rendersHelpContent_andFollowsConfig();

  private:
    QTemporaryDir* m_dir = nullptr;
    Config* m_config = nullptr;
    OsdOverlay* m_overlay = nullptr;
};
