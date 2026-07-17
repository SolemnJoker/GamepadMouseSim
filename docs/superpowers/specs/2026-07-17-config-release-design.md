# Config & Release Standard — Design Spec

**Date:** 2026-07-14
**Status:** Approved for implementation
**Owner:** GamepadMouseSim maintainers
**Sub-project:** 4 of 5 under the project-standards initiative

---

## 1. Scope

### 1.1 In scope

- Add `schema_version` field to `config/default_config.json`
- Add `kCurrentConfigSchemaVersion` constant to `src/core/Types.h`
- Modify `Config::load()` to detect version mismatch and overwrite with default config
- ctest validation for version-mismatch fallback

### 1.2 Out of scope

| Concern | Reason |
|---|---|
| Semantic versioning for the application | User did not select this |
| Version number single source of truth | User did not select this |
| Installer/deploy script conventions | User did not select this |
| Migration scripts (per-version transforms) | User chose "overwrite + replace" strategy |

---

## 2. Design

### 2.1 New constant

In `src/core/Types.h`, alongside `kMaxGamepads`:

```cpp
constexpr int kCurrentConfigSchemaVersion = 1;
```

### 2.2 Updated default config

Add `"schema_version": 1` to `config/default_config.json` as the top-level first key:

```json
{
  "schema_version": 1,
  "monitoring": { ... },
  ...
}
```

### 2.3 Updated Config::load()

New private helper `Config::loadDefault()` reads `default_config.json` and writes to `m_filePath`:

```cpp
bool Config::loadDefault() {
    QString defaultPath = QCoreApplication::applicationDirPath() + "/config/default_config.json";
    QFile defaultFile(defaultPath);
    if (!defaultFile.open(QIODevice::ReadOnly))
        return false;
    m_data = QJsonDocument::fromJson(defaultFile.readAll()).object();
    defaultFile.close();
    return save();
}
```

`Config::load()` checks the version after reading:

```cpp
bool Config::load(const QString& path) {
    bool fileExists = ...;  // existing file-read logic
    if (!fileExists) {
        // Fallback: load default and write a writable copy
        loadDefault();
        return false;
    }

    // ... existing read & parse logic ...

    const int version = value("schema_version", 0).toInt();
    if (version < kCurrentConfigSchemaVersion) {
        qInfo() << "Config schema version" << version
                << "<" << kCurrentConfigSchemaVersion
                << "- overwriting with default";
        loadDefault();
        // Don't return false — the config was successfully loaded,
        // just replaced with a newer version.
    }

    // ... existing watcher setup ...
    return true;
}
```

### 2.4 ctest

Add `schemaVersion_usesDefaultOnMismatch` to `tests/test_config_autostart.cpp`:

```cpp
void schemaVersion_usesDefaultOnMismatch() {
    // Create a config with old schema version
    QTemporaryFile oldConfig(this);
    oldConfig.open();
    oldConfig.write(R"({"schema_version": 0, "autostart": true})");
    oldConfig.close();

    Config cfg(this);
    QVERIFY(cfg.load(oldConfig.fileName()));
    // After loading, the config should have been replaced with defaults
    // from default_config.json — schema_version should be the current one.
    QCOMPARE(cfg.value("schema_version", 0).toInt(), kCurrentConfigSchemaVersion);
}
```

---

## 3. Verification

- Build: `cmake --build build --config Release` — zero new warnings
- Test: `ctest --test-dir build -R test_config_autostart` — all 5 cases pass (was 4)
- Full suite: `ctest --test-dir build` — 5 test executables pass

---

## Appendix A — Cross-references

- **Code-architecture spec**: §2.3 (Config is the single bridge) defines the Config bootstrap order.
- **User-docs spec**: §2.2 (all enum values must have Chinese labels) — no overlap.
- **Commit-workflow spec**: §4 (PULL_REQUEST_TEMPLATE checklist) — no overlap.