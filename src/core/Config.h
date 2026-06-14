#pragma once

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileSystemWatcher>
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

signals:
    void configChanged();

private slots:
    void onFileChanged(const QString& path);

private:
    QVariant getNestedValue(const QJsonObject& obj, const QStringList& keys) const;
    void setNestedValue(QJsonObject& obj, const QStringList& keys, const QVariant& value);

    QString m_filePath;
    QJsonObject m_data;
    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;
};
