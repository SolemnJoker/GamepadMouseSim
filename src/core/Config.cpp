#include "Config.h"
#include "core/MappingDefaults.h"
#include "core/Types.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonParseError>

namespace {

// qrc 打包的出厂默认配置(resources.qrc alias="config/default_config.json")。
constexpr const char* kBuiltinDefaultPath = ":/config/default_config.json";

} // namespace

Config::Config(QObject* parent) : QObject(parent) {
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(300);
    connect(&m_debounceTimer, &QTimer::timeout, this, &Config::configChanged);
}

QJsonObject Config::builtinDefaultConfig() {
    QFile res(QString::fromLatin1(kBuiltinDefaultPath));
    if (res.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(res.readAll()).object();
        res.close();
        if (!obj.isEmpty())
            return obj;
    }
    qWarning() << "Builtin default config resource unavailable, using MappingDefaults skeleton";
    QJsonObject obj = MappingDefaults::skeletonMouseMode();
    obj["schema_version"] = kCurrentConfigSchemaVersion;
    return obj;
}

bool Config::load(const QString& path) {
    m_filePath = path;
    bool loaded = false;

    // 1) exe 旁可写 config.json;坏 JSON 按出厂重置处理(design D4)。
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error == QJsonParseError::NoError) {
            m_data = doc.object();
            loaded = true;
        } else {
            qWarning() << "Config JSON parse failed:" << err.errorString()
                       << "- resetting to factory defaults";
        }
    }

    // 2) qrc 内嵌默认配置 + 自动写出可写 config.json(裸 exe 首启动)。
    bool migrated = false;
    if (loaded)
        migrated = migrateIfNeeded();
    if (!loaded) {
        m_data = builtinDefaultConfig();
        if (!m_data.isEmpty()) {
            loaded = true; // 内置兜底视为加载成功(数据完整,文件随 save 落盘)
        }
        migrateIfNeeded();
    }
    reconcileProfileMirror();
    if (!loaded || migrated) {
        if (!save())
            qWarning() << "Config: unable to write" << path << "- running with in-memory config";
    }
    attachWatcher(path);
    return loaded;
}

bool Config::migrateIfNeeded() {
    const int version = value("schema_version", 0).toInt();
    if (version >= kCurrentConfigSchemaVersion)
        return false;

    qInfo() << "Config schema version" << version << "->" << kCurrentConfigSchemaVersion
            << "- incremental merge";
    bool changed = false;

    // v1 -> v2:现有 mouse_mode 原样成为首个 profile("default")。
    if (version < 2 && !value("profiles").toJsonValue().isObject()) {
        QJsonObject prof;
        prof["mouse_mode"] = value("mouse_mode").toJsonValue().toObject();
        QJsonObject list;
        list["default"] = prof;
        QJsonArray order;
        order.append("default");
        QJsonObject profiles;
        profiles["active"] = QStringLiteral("default");
        profiles["order"] = order;
        profiles["list"] = list;
        m_data["profiles"] = profiles;
        changed = true;
    }

    // 增量合并:内置默认为骨架,用户已有键全保留,缺失键补默认。
    const QJsonObject merged = MappingDefaults::mergedDeep(builtinDefaultConfig(), m_data);
    if (merged != m_data)
        changed = true;
    m_data = merged;
    m_data["schema_version"] = kCurrentConfigSchemaVersion;
    return changed;
}

void Config::reconcileProfileMirror() {
    const QJsonObject mouseMode = value("mouse_mode").toJsonValue().toObject();
    if (mouseMode.isEmpty())
        return;
    const QString active = activeProfileName();
    const QJsonObject profMouseMode =
        value("profiles.list." + active + ".mouse_mode").toJsonValue().toObject();
    if (mouseMode != profMouseMode)
        updateActiveProfileFromMouseMode();
}

void Config::attachWatcher(const QString& path) {
    if (m_watcher.files().contains(path))
        m_watcher.removePath(path);
    m_watcher.addPath(path);
    disconnect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Config::onFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Config::onFileChanged);
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

QStringList Config::profileOrder() const {
    QStringList out;
    const QVariant v = value("profiles.order");
    if (v.canConvert<QVariantList>()) {
        for (const QVariant& item : v.toList())
            out << item.toString();
    }
    return out;
}

QString Config::activeProfileName() const {
    return value("profiles.active", QStringLiteral("default")).toString();
}

void Config::applyActiveProfile() {
    const QString active = activeProfileName();
    const QJsonObject profMouseMode =
        value("profiles.list." + active + ".mouse_mode").toJsonValue().toObject();
    // 展开:profile 缺子键补默认;空 profile 代表"出厂方案" → 展开为骨架。
    // 加载/外部编辑路径不走这里(走 reconcileProfileMirror 的采集方向)。
    const QJsonObject merged = MappingDefaults::mergedMouseMode(profMouseMode);
    setNestedValue(m_data, {"mouse_mode"}, merged);
}

void Config::setActiveProfile(const QString& name) {
    if (!value("profiles.list." + name).toJsonValue().isObject())
        return;
    beginBatch();
    setValue("profiles.active", name);
    applyActiveProfile();
    endBatch();
}

bool Config::createProfile(const QString& name) {
    if (name.isEmpty() || value("profiles.list." + name).toJsonValue().isObject())
        return false;
    beginBatch();
    // 以当前生效方案为模板复制。
    QJsonObject prof;
    prof["mouse_mode"] = value("mouse_mode").toJsonValue().toObject();
    setValue("profiles.list." + name, prof);
    QStringList order = profileOrder();
    order << name;
    setValue("profiles.order", order);
    endBatch();
    return true;
}

bool Config::renameProfile(const QString& oldName, const QString& newName) {
    if (newName.isEmpty() || oldName == newName ||
        !value("profiles.list." + oldName).toJsonValue().isObject())
        return false;
    if (value("profiles.list." + newName).toJsonValue().isObject())
        return false;
    beginBatch();
    setValue("profiles.list." + newName,
             value("profiles.list." + oldName).toJsonValue().toObject());
    QJsonObject list = value("profiles.list").toJsonValue().toObject();
    list.remove(oldName);
    setNestedValue(m_data, {"profiles", "list"}, list);
    QStringList order = profileOrder();
    order.replaceInStrings(oldName, newName);
    setValue("profiles.order", order);
    if (activeProfileName() == oldName)
        setValue("profiles.active", newName);
    endBatch();
    return true;
}

bool Config::removeProfile(const QString& name) {
    QStringList order = profileOrder();
    if (!value("profiles.list." + name).toJsonValue().isObject() || order.size() <= 1)
        return false; // 至少保留一套
    beginBatch();
    QJsonObject list = value("profiles.list").toJsonValue().toObject();
    list.remove(name);
    setNestedValue(m_data, {"profiles", "list"}, list);
    order.removeAll(name);
    setValue("profiles.order", order);
    if (activeProfileName() == name) {
        setValue("profiles.active", order.first());
        applyActiveProfile();
    }
    endBatch();
    return true;
}

void Config::updateActiveProfileFromMouseMode() {
    const QJsonObject mouseMode = value("mouse_mode").toJsonValue().toObject();
    if (mouseMode.isEmpty())
        return;
    beginBatch();
    setValue("profiles.list." + activeProfileName() + ".mouse_mode", mouseMode);
    endBatch();
}

void Config::restoreFactoryDefaults() {
    m_data = builtinDefaultConfig();
    migrateIfNeeded();
    reconcileProfileMirror();
    if (!save())
        qWarning() << "Config: unable to write" << m_filePath << "- factory defaults in memory";
    emit configChanged(); // 立即广播,不等 300ms watcher 去抖
}

void Config::onFileChanged(const QString& path) {
    Q_UNUSED(path);
    if (!m_debounceTimer.isActive()) {
        m_debounceTimer.start();
    }
    QTimer::singleShot(350, this, [this, filePath = path]() {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            if (err.error == QJsonParseError::NoError) {
                m_data = doc.object();
                migrateIfNeeded();
                reconcileProfileMirror(); // 外部编辑以 mouse_mode 为权威采集
            }
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
