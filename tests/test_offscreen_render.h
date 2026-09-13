#pragma once

#include <QtTest>

// L3 可行性 spike(design.md D7):offscreen QPA 下普通 QWidget + grab()
// 是否可用(仓库已知坑仅限 QSystemTrayIcon)。若本测试崩溃/失败,L3 层
// 降级为 L1 数据断言 + L5 目测,结论记入 follow-up 文档。
class TestOffscreenRender : public QObject {
    Q_OBJECT
  private slots:
    void grab_basic();
};
