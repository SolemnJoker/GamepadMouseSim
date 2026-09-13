#include "ConfigSelftest.h"
#include "Config.h"
#include "MappingDefaults.h"
#include "Types.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <cstdio>

namespace {

int g_failures = 0;

void report(const char* name, bool ok) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    std::fflush(stdout);
    if (!ok)
        ++g_failures;
}

bool resourceReadable() {
    QFile res(QStringLiteral(":/config/default_config.json"));
    if (!res.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject obj = QJsonDocument::fromJson(res.readAll()).object();
    res.close();
    return obj.contains("schema_version") && obj.contains("mouse_mode");
}

bool loadChainGeneratesWritableConfig() {
    QTemporaryDir dir;
    if (!dir.isValid())
        return false;
    const QString path = dir.filePath("config.json");
    Config cfg;
    if (!cfg.load(path))
        return false;
    if (!QFile::exists(path))
        return false;
    return cfg.value("schema_version").toInt() == kCurrentConfigSchemaVersion &&
           !cfg.value("mouse_mode.button_mapping.A").toString().isEmpty() &&
           !cfg.profileOrder().isEmpty();
}

bool incrementalMigrationKeepsUserKeys() {
    QTemporaryDir dir;
    if (!dir.isValid())
        return false;
    const QString path = dir.filePath("config.json");
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return false;
        f.write(
            R"({"schema_version": 1, "mouse_mode": {"button_mapping": {"A": "MouseRightClick"}}})");
        f.close();
    }
    Config cfg;
    if (!cfg.load(path))
        return false;
    return cfg.value("schema_version").toInt() == kCurrentConfigSchemaVersion &&
           cfg.value("mouse_mode.button_mapping.A").toString() ==
               QLatin1String("MouseRightClick") &&
           cfg.value("mouse_mode.button_mapping.X").toString() == QLatin1String("MouseMiddleClick");
}

bool profileSwitchExpandsMirror() {
    QTemporaryDir dir;
    if (!dir.isValid())
        return false;
    Config cfg;
    if (!cfg.load(dir.filePath("config.json")))
        return false;
    if (!cfg.createProfile(QStringLiteral("selftest")))
        return false;
    cfg.setActiveProfile(QStringLiteral("selftest"));
    cfg.beginBatch();
    cfg.setValue("mouse_mode.button_mapping.Y", QStringLiteral("Ctrl+V"));
    cfg.endBatch();
    cfg.updateActiveProfileFromMouseMode();
    cfg.setActiveProfile(QStringLiteral("default"));
    if (cfg.value("mouse_mode.button_mapping.Y").toString() != QLatin1String("Enter"))
        return false;
    cfg.setActiveProfile(QStringLiteral("selftest"));
    return cfg.value("mouse_mode.button_mapping.Y").toString() == QLatin1String("Ctrl+V");
}

bool mergedViewKeepsDefaultBindings() {
    return MappingDefaults::mergedLayer("LT", QJsonObject()).value("Menu") ==
           ButtonAction::ShowKeyboard;
}

bool defaultConfigMatchesMappingDefaults() {
    QFile res(QStringLiteral(":/config/default_config.json"));
    if (!res.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject root = QJsonDocument::fromJson(res.readAll()).object();
    res.close();
    const QJsonObject jsonDirect =
        root.value("mouse_mode").toObject().value("button_mapping").toObject();
    const QMap<QString, ButtonAction> defaults = MappingDefaults::directDefaults();
    for (auto it = jsonDirect.begin(); it != jsonDirect.end(); ++it) {
        if (!defaults.contains(it.key()) ||
            stringToAction(it.value().toString()) != defaults.value(it.key()))
            return false;
    }
    return true;
}

} // namespace

bool runConfigSelftest() {
    report("qrc builtin default config readable", resourceReadable());
    report("load chain generates writable config on first launch",
           loadChainGeneratesWritableConfig());
    report("incremental migration preserves user keys", incrementalMigrationKeepsUserKeys());
    report("profile switch expands mirror", profileSwitchExpandsMirror());
    report("merged view keeps default bindings (LT.Menu=ShowKeyboard)",
           mergedViewKeepsDefaultBindings());
    report("default_config.json matches MappingDefaults", defaultConfigMatchesMappingDefaults());
    return g_failures == 0;
}
