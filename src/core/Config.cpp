#include "Config.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>

Config::Config(QObject* parent) : QObject(parent) {
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(300);
    connect(&m_debounceTimer, &QTimer::timeout, this, &Config::configChanged);
}

bool Config::load(const QString& path) {
    m_filePath = path;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QFile defaultFile(QCoreApplication::applicationDirPath() + "/config/default_config.json");
        if (defaultFile.open(QIODevice::ReadOnly)) {
            m_data = QJsonDocument::fromJson(defaultFile.readAll()).object();
            defaultFile.close();
            save();
        }
        return false;
    }

    m_data = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    if (m_watcher.files().contains(path)) {
        m_watcher.removePath(path);
    }
    m_watcher.addPath(path);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Config::onFileChanged);

    return true;
}

bool Config::save() {
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(m_data).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QVariant Config::value(const QString& key, const QVariant& defaultValue) const {
    QStringList keys = key.split('.');
    QVariant result = getNestedValue(m_data, keys);
    return result.isValid() ? result : defaultValue;
}

void Config::setValue(const QString& key, const QVariant& value) {
    QStringList keys = key.split('.');
    setNestedValue(m_data, keys, value);
    if (m_batchDepth == 0) {
        save();
    }
}

void Config::beginBatch() {
    ++m_batchDepth;
}

void Config::endBatch() {
    if (--m_batchDepth == 0) {
        save();
    }
}

void Config::onFileChanged(const QString& path) {
    Q_UNUSED(path);
    if (!m_debounceTimer.isActive()) {
        m_debounceTimer.start();
    }
    QTimer::singleShot(350, this, [this, filePath = path]() {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            m_data = QJsonDocument::fromJson(file.readAll()).object();
            file.close();
        }
        if (!m_watcher.files().contains(filePath)) {
            m_watcher.addPath(filePath);
        }
    });
}

QVariant Config::getNestedValue(const QJsonObject& obj, const QStringList& keys) const {
    if (keys.isEmpty())
        return QVariant();
    if (keys.size() == 1)
        return obj.value(keys.first()).toVariant();

    QJsonObject child = obj.value(keys.first()).toObject();
    return getNestedValue(child, keys.mid(1));
}

void Config::setNestedValue(QJsonObject& obj, const QStringList& keys, const QVariant& value) {
    if (keys.size() == 1) {
        obj[keys.first()] = QJsonValue::fromVariant(value);
        return;
    }
    QJsonObject child = obj.value(keys.first()).toObject();
    setNestedValue(child, keys.mid(1), value);
    obj[keys.first()] = child;
}
