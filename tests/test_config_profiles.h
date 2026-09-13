#pragma once

#include <QTemporaryDir>
#include <QtTest>

class Config;

// profile 展开/回写/管理规则(design D3;spec:Profile 数据模型与管理入口)。
class TestConfigProfiles : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void setActiveProfile_expandsMirror();
    void emptyProfile_mouseModeStaysAuthoritative();
    void createProfile_copiesCurrent_andAppendsOrder();
    void removeProfile_keepsAtLeastOne_andSwitchesWhenCurrent();
    void renameProfile_updatesOrderAndActive();
    void updateFromMouseMode_writesBackToActive_only();

  private:
    QTemporaryDir* m_dir = nullptr;
    Config* m_config = nullptr;
};
