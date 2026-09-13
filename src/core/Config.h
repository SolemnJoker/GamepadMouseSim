#pragma once

#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariant>

class Config : public QObject {
    Q_OBJECT
  public:
    explicit Config(QObject* parent = nullptr);

    bool load(const QString& path);
    bool save();
    QString filePath() const { return m_filePath; }

    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    void beginBatch();
    void endBatch();

    // --- profiles(设计 D3/D6:Config 是 profile 的唯一写入口) ---
    QStringList profileOrder() const;
    QString activeProfileName() const;
    // 切换生效方案:写入 active + 把该方案展开镜像到 mouse_mode.*。
    void setActiveProfile(const QString& name);
    // 以当前 active 方案为模板新建;名字重复或为空时失败。
    bool createProfile(const QString& name);
    bool renameProfile(const QString& oldName, const QString& newName);
    // 约束:至少保留一套;删除当前项时自动切到剩余首项。
    bool removeProfile(const QString& name);
    // 设置界面保存 mouse_mode.* 后调用:回写到 active profile 保持镜像一致。
    void updateActiveProfileFromMouseMode();
    // 全局一键恢复出厂(托盘逃生门):整个配置替换为内置默认并落盘,
    // 立即广播热生效。坏 JSON 的自动退化走 load 路径,这里是用户主动重置。
    void restoreFactoryDefaults();
    // 把 profiles.list.<active>.mouse_mode 展开镜像到 mouse_mode.*(运行时
    // 子系统只读 mouse_mode.*,对 profile 无感知)。空 profile = 出厂方案。
    void applyActiveProfile();
    // 镜像对齐(采集方向):mouse_mode 与 active profile 不一致时,把
    // mouse_mode 采集进 profile——加载与外部编辑场景中鼠标节是权威。
    void reconcileProfileMirror();

  signals:
    void configChanged();

  private slots:
    void onFileChanged(const QString& path);

  private:
    // 增量迁移到当前 schema(design D4):保留用户键、缺失补默认、
    // v1 的 mouse_mode 原样成为 profiles.list.default。返回是否有变更。
    bool migrateIfNeeded();
    void attachWatcher(const QString& path);
    // 内置默认配置(qrc 打包的 default_config.json;读取失败时用
    // MappingDefaults 骨架兜底)。
    static QJsonObject builtinDefaultConfig();

    QVariant getNestedValue(const QJsonObject& obj, const QStringList& keys) const;
    void setNestedValue(QJsonObject& obj, const QStringList& keys, const QVariant& value);

    QString m_filePath;
    QJsonObject m_data;
    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;
    int m_batchDepth = 0;
};
