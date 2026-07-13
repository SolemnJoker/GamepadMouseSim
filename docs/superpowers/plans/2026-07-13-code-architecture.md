# Code Architecture & Quality Standard — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring the GamepadMouseSim C++/Qt codebase into compliance with the rules defined in `docs/superpowers/specs/2026-07-13-code-architecture-design.md`, add a minimal Qt Test scaffold, and ship `.clang-format` + helper scripts so future commits can self-check formatting.

**Architecture:** Four sequential PRs (PR1 → PR2 → PR3 → PR4). Each PR is small, independently mergeable, and verified by a documented check (manual smoke, `ctest`, or `scripts/check-format.ps1`). The four PRs together close every spec gap identified during the 2026-07-13 audit.

**Tech Stack:** Qt 6.8.3 (Widgets, Svg, Test), C++17, CMake 3.20+, MSVC 2022, Qt Test framework. No new third-party dependencies.

**Reference spec:** `docs/superpowers/specs/2026-07-13-code-architecture-design.md` (commit `2d01f48`).

**Working directory:** `E:\project\sbgj`. Build command (from repo root):

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

(Pre-condition: `cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..` already run inside `build/`.)

---

## File Structure

### New files

| Path                              | Purpose                                                |
|-----------------------------------|--------------------------------------------------------|
| `.clang-format`                   | Formatter config (root)                                |
| `.editorconfig`                   | Editor indent/EOL config (root)                        |
| `scripts/format.ps1`              | One-shot formatter wrapper                             |
| `scripts/check-format.ps1`        | Pre-commit dry-run check                               |
| `tests/CMakeLists.txt`            | Test sub-build                                         |
| `tests/test_main.cpp`             | Qt Test entry point                                    |
| `tests/test_config_autostart.cpp` | 4 cases covering PR1's Config contract                 |

### Modified files (PR-ordered)

| PR | File                                            | Change                                                                  |
|----|-------------------------------------------------|-------------------------------------------------------------------------|
| 1  | `src/core/AutoStart.h`                          | Replace `isEnabled/setEnabled` API with `applyToRegistry(bool)`         |
| 1  | `src/core/AutoStart.cpp`                        | Single helper body (former `setEnabled` body)                           |
| 1  | `src/ui/SettingsDialog.cpp`                     | Drop `core/AutoStart.h` include; use `m_config->value/setValue("autostart", …)` |
| 1  | `src/app/Application.cpp`                       | Replace autostart block; extend `configChanged` lambda                  |
| 1  | `config/default_config.json`                    | Add `"autostart": false`                                                |
| 1  | `CMakeLists.txt`                                | Extract `src_core` static lib; add `enable_testing()` + `add_subdirectory(tests)` |
| 2  | `src/core/Types.h`                              | Add `kXInputThumbMax`, `kXInputTriggerMax`, `kXInputDeadzoneMax`, `kMsPerSecond` |
| 2  | `src/gamepad/GamepadPoller.h`                   | `m_prevState` → `std::array<GamepadState, kMaxGamepads>`                |
| 2  | `src/gamepad/GamepadPoller.cpp`                 | Replace literals; loop bound → `kMaxGamepads`                           |
| 2  | `src/core/ModeManager.cpp`                      | `* 1000` → `* kMsPerSecond`                                             |
| 2  | `src/core/AutoModeController.cpp`               | `* 1000` → `* kMsPerSecond` (3 sites)                                   |
| 3  | `src/app/Application.cpp`                       | `qDebug` → `qWarning`; drop `"WARNING:"` substring                       |
| 3  | `src/main.cpp`                                  | `qDebug` → `qCritical`                                                  |
| 4  | (all `*.cpp`, `*.h` under `src/` and `tests/`)  | One-time `clang-format -i`                                              |

---

## Task 1: PR1 — AutoStart via Config hot-reload

### Task 1.1: Add `src_core` static library in CMakeLists

**Files:**
- Modify: `CMakeLists.txt:14-57`

- [ ] **Step 1: Refactor SOURCES/HEADERS so `src/core/*.cpp` lives in its own static lib**

In `CMakeLists.txt`, replace the `set(SOURCES …)` and `set(HEADERS …)` blocks (lines 14-57) with a `src_core` static library and a reduced main-target list. The new shape:

```cmake
# Static library for code that needs to be linkable into tests.
add_library(src_core STATIC
    src/core/Config.cpp
    src/core/Types.cpp
)
target_include_directories(src_core PUBLIC src)
target_link_libraries(src_core PUBLIC Qt6::Core)

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/core/ModeManager.cpp
    src/core/ProcessDetector.cpp
    src/core/AutoStart.cpp
    src/core/SysUsage.cpp
    src/core/GameDetector.cpp
    src/core/AutoModeController.cpp
    src/gamepad/GamepadPoller.cpp
    src/gamepad/ComboKeyDetector.cpp
    src/input/InputMapper.cpp
    src/input/MouseMapper.cpp
    src/input/KeyboardMapper.cpp
    src/ui/SystemTray.cpp
    src/ui/OsdOverlay.cpp
    src/ui/SettingsDialog.cpp
    src/win/XInputWrapper.cpp
    src/win/SendInputHelper.cpp
)

set(HEADERS
    src/app/Application.h
    src/core/ModeManager.h
    src/core/ProcessDetector.h
    src/core/AutoStart.h
    src/core/SysUsage.h
    src/core/GameDetector.h
    src/core/AutoModeController.h
    src/gamepad/GamepadPoller.h
    src/gamepad/ComboKeyDetector.h
    src/input/InputMapper.h
    src/input/MouseMapper.h
    src/input/KeyboardMapper.h
    src/ui/SystemTray.h
    src/ui/OsdOverlay.h
    src/ui/SettingsDialog.h
    src/win/XInputWrapper.h
    src/win/SendInputHelper.h
)
```

Notes:
- `Config.cpp` / `Types.cpp` / `Config.h` / `Types.h` move into `src_core` and out of `SOURCES` / `HEADERS`.
- `add_library(src_core STATIC ...)` is placed **above** the existing `set(SOURCES ...)` block.

- [ ] **Step 2: Add `enable_testing()` + link `src_core` into the main target**

Right after the `add_library(src_core …)` block (before `set(SOURCES …)`), insert:

```cmake
enable_testing()
```

In the `target_link_libraries(${PROJECT_NAME} PRIVATE …)` block (lines 78-85), prepend `src_core`:

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE
    src_core
    Qt6::Widgets
    Qt6::Svg
    xinput
    oleaut32 imm32 opengl32 version winmm
    ws2_32 uuid netapi32 userenv dwmapi
    dxgi
)
```

- [ ] **Step 3: Append `add_subdirectory(tests)` at the bottom**

At the end of `CMakeLists.txt` (after the `install(TARGETS …)` block), add:

```cmake
add_subdirectory(tests)
```

- [ ] **Step 4: Build and confirm**

Run:

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: build succeeds; no new warnings compared to baseline commit `7a58792`.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: split src_core static lib + enable_testing (no-op for prod)"
```

### Task 1.2: Refactor `AutoStart` to a single helper

**Files:**
- Modify: `src/core/AutoStart.h`
- Modify: `src/core/AutoStart.cpp`

- [ ] **Step 1: Replace `AutoStart.h` public API**

Overwrite `src/core/AutoStart.h` to:

```cpp
#pragma once

#include <QString>

// Manages boot auto-start via HKCU\...\Run registry key (no admin needed).
// The single public entry point is applyToRegistry(); the surrounding
// config/subsystem code is responsible for deciding *when* to call it.
class AutoStart {
public:
    static const QString kRunKey;       // "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
    static const QString kValueName;    // "GamepadMouseSim"

    static void applyToRegistry(bool enabled);
    static QString executablePath();
};
```

- [ ] **Step 2: Replace `AutoStart.cpp` body**

Overwrite `src/core/AutoStart.cpp` to:

```cpp
#include "AutoStart.h"
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

const QString AutoStart::kRunKey   = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const QString AutoStart::kValueName = "GamepadMouseSim";

QString AutoStart::executablePath() {
    // QCoreApplication::applicationFilePath() returns the .exe path with native separators.
    QString path = QCoreApplication::applicationFilePath();
    return path;
}

void AutoStart::applyToRegistry(bool enabled) {
    QSettings settings(kRunKey, QSettings::NativeFormat);
    if (enabled) {
        // Quote the path so spaces in the directory don't break execution.
        QString exe = executablePath();
        settings.setValue(kValueName, "\"" + exe + "\"");
        qDebug() << "Autostart enabled:" << exe;
    } else {
        if (settings.contains(kValueName)) {
            settings.remove(kValueName);
            qDebug() << "Autostart disabled";
        }
    }
    settings.sync();
}
```

- [ ] **Step 3: Build to confirm `SettingsDialog` / `Application` break (expected)**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: **FAIL** with `unresolved external symbol AutoStart::isEnabled` / `AutoStart::setEnabled`. This is expected; the next two tasks fix the callers.

- [ ] **Step 4: Commit (don't push; PR1 is not done yet)**

```bash
git add src/core/AutoStart.h src/core/AutoStart.cpp
git commit -m "refactor(core): collapse AutoStart into applyToRegistry"
```

### Task 1.3: Update `SettingsDialog` to use Config

**Files:**
- Modify: `src/ui/SettingsDialog.cpp:1-2, 354, 421-426`

- [ ] **Step 1: Drop the `AutoStart.h` include**

In `src/ui/SettingsDialog.cpp:1-2`, replace:

```cpp
#include "SettingsDialog.h"
#include "core/Config.h"
#include "core/AutoStart.h"
```

with:

```cpp
#include "SettingsDialog.h"
#include "core/Config.h"
```

- [ ] **Step 2: Read autostart state from Config**

In `src/ui/SettingsDialog.cpp`, replace line 354:

```cpp
m_autostart->setChecked(AutoStart::isEnabled());
```

with:

```cpp
m_autostart->setChecked(m_config->value("autostart", false).toBool());
```

- [ ] **Step 3: Drop the direct registry write in `saveValues()`**

In `src/ui/SettingsDialog.cpp`, replace the block at lines 421-426:

```cpp
    // Boot autostart (registry, not the JSON field)
    bool as = m_autostart->isChecked();
    if (as != AutoStart::isEnabled()) {
        AutoStart::setEnabled(as);
    }
    m_config->setValue("autostart", as);
```

with:

```cpp
    // Boot autostart: write through Config so configChanged fans out
    // to Application, which is the sole writer of the registry.
    m_config->setValue("autostart", m_autostart->isChecked());
```

- [ ] **Step 4: Commit**

```bash
git add src/ui/SettingsDialog.cpp
git commit -m "refactor(ui): read/write autostart via Config (no AutoStart dependency)"
```

### Task 1.4: Update `Application` to sync registry from `configChanged`

**Files:**
- Modify: `src/app/Application.cpp:106-123`

- [ ] **Step 1: Extend the existing `configChanged` lambda**

Replace the `connect(&m_config, &Config::configChanged, this, [this]() { ... })` block at lines 106-112 with:

```cpp
    connect(&m_config, &Config::configChanged, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->onConfigChanged();
            m_inputMappers[i]->onConfigChanged();
        }
        m_autoController->onConfigChanged();

        // Autostart is the only key this file owns; sync the registry
        // whenever it changes. Skip if the value hasn't changed to avoid
        // redundant registry writes on unrelated config edits.
        const bool wantAutostart = m_config.value("autostart", false).toBool();
        if (wantAutostart != m_lastAutostart) {
            m_lastAutostart = wantAutostart;
            AutoStart::applyToRegistry(wantAutostart);
        }
    });
```

- [ ] **Step 2: Replace the startup autostart block**

Replace the block at lines 114-123:

```cpp
    // Apply the configured boot autostart state on every launch.
    if (m_config.value("autostart", false).toBool()) {
        if (!AutoStart::isEnabled()) {
            AutoStart::setEnabled(true);
        }
    } else {
        if (AutoStart::isEnabled()) {
            AutoStart::setEnabled(false);
        }
    }
```

with:

```cpp
    // Apply the configured boot autostart state on every launch.
    m_lastAutostart = m_config.value("autostart", false).toBool();
    AutoStart::applyToRegistry(m_lastAutostart);
```

- [ ] **Step 3: Add `m_lastAutostart` member to `Application.h`**

In `src/app/Application.h`, find the private section (the one that holds
`m_autoController`, `m_settingsDialog`, etc.) and add a new line:

```cpp
    bool m_lastAutostart = false;
```

- [ ] **Step 4: Build to confirm everything compiles**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: clean build, zero new warnings.

- [ ] **Step 5: Commit**

```bash
git add src/app/Application.h src/app/Application.cpp
git commit -m "fix(app): own boot autostart; UI no longer touches AutoStart directly"
```

### Task 1.5: Verify default autostart key in bundled config

**Files:**
- Inspect only: `config/default_config.json` (no change expected)

- [ ] **Step 1: Confirm `"autostart": false` exists at top level**

```bash
grep -n '"autostart"' config/default_config.json
```

Expected: a single match around line 87, `"autostart": false,`. (Audit on 2026-07-13 confirmed the file already carries the key.) If absent, add it as a top-level entry alongside `monitoring`, `combo_key`, `mouse_mode`, `osd`, `auto_switch`.

- [ ] **Step 2: No commit required**

If the key is already present (expected case), do not commit. Only commit if you had to add it.

### Task 1.6: Add tests/ scaffold

**Files:**
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_main.cpp`
- Create: `tests/test_config_autostart.cpp`

- [ ] **Step 1: Create `tests/CMakeLists.txt`**

Write to `tests/CMakeLists.txt`:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(test_config_autostart
    test_main.cpp
    test_config_autostart.cpp
)
target_link_libraries(test_config_autostart PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_config_autostart COMMAND test_config_autostart)
```

- [ ] **Step 2: Create `tests/test_main.cpp`**

Write to `tests/test_main.cpp`:

```cpp
#include <QtTest>
#include "test_config_autostart.h"

QTEST_MAIN(TestConfigAutostart)
```

- [ ] **Step 3: Create `tests/test_config_autostart.h`**

Write to `tests/test_config_autostart.h`:

```cpp
#pragma once

#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "core/Config.h"

class TestConfigAutostart : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void setValue_emitsConfigChanged();
    void setValue_writesToDisk();
    void reloadReReadsValue();
    void nestedPathRoundTrip();

private:
    QTemporaryFile* m_tmpFile = nullptr;
    Config* m_config = nullptr;
};
```

- [ ] **Step 4: Create `tests/test_config_autostart.cpp`**

Write to `tests/test_config_autostart.cpp`:

```cpp
#include "test_config_autostart.h"
#include <QFile>
#include <QJsonParseError>

void TestConfigAutostart::init() {
    m_tmpFile = new QTemporaryFile(this);
    QVERIFY(m_tmpFile->open());
    m_tmpFile->write("{}");
    m_tmpFile->close();

    m_config = new Config(this);
    QVERIFY(m_config->load(m_tmpFile->fileName()));
}

void TestConfigAutostart::cleanup() {
    delete m_config;
    m_config = nullptr;
    delete m_tmpFile;
    m_tmpFile = nullptr;
}

void TestConfigAutostart::setValue_emitsConfigChanged() {
    QSignalSpy spy(m_config, &Config::configChanged);
    m_config->setValue("autostart", true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_config->value("autostart", false).toBool(), true);
}

void TestConfigAutostart::setValue_writesToDisk() {
    m_config->setValue("autostart", true);

    QFile f(m_tmpFile->fileName());
    QVERIFY(f.open(QIODevice::ReadOnly));
    QJsonParseError err;
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll(), &err).object();
    f.close();
    QVERIFY2(err.error == QJsonParseError::NoError, qPrintable(err.errorString()));
    QCOMPARE(obj.value("autostart").toBool(), true);
}

void TestConfigAutostart::reloadReReadsValue() {
    // Externally overwrite the file with a different autostart value.
    {
        QFile f(m_tmpFile->fileName());
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(R"({"autostart": true})");
        f.close();
    }

    QSignalSpy spy(m_config, &Config::configChanged);
    // QFileSystemWatcher + 300 ms debounce + 350 ms singleShot
    QTest::qWait(800);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_config->value("autostart", false).toBool(), true);
}

void TestConfigAutostart::nestedPathRoundTrip() {
    m_config->setValue("a.b.c", 42);
    m_config->setValue("a.x", "untouched");

    QCOMPARE(m_config->value("a.b.c", -1).toInt(), 42);
    QCOMPARE(m_config->value("a.x").toString(), QStringLiteral("untouched"));

    // The setValue for "a.b.c" must NOT clobber the sibling "a.x".
    QJsonObject obj = m_config->value("a").toJsonObject();
    QCOMPARE(obj.value("x").toString(), QStringLiteral("untouched"));
    QCOMPARE(obj.value("b").toObject().value("c").toInt(), 42);
}
```

- [ ] **Step 5: Reconfigure CMake and build the test target**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release'
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --target test_config_autostart --config Release'
```

Expected: test binary compiles; no link errors.

- [ ] **Step 6: Run the tests**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && ctest --test-dir build -R test_config_autostart --output-on-failure'
```

Expected: 4/4 tests pass.

- [ ] **Step 7: Commit**

```bash
git add tests/CMakeLists.txt tests/test_main.cpp tests/test_config_autostart.h tests/test_config_autostart.cpp
git commit -m "test(core): Qt Test scaffold covering Config autostart round-trip"
```

### Task 1.7: Manual smoke + registry round-trip

- [ ] **Step 1: Launch the built exe**

```bash
build/GamepadMouseSim.exe
```

Expected: tray icon appears; no console errors.

- [ ] **Step 2: Toggle autostart via Settings dialog**

Right-click tray → Settings → check "Boot autostart" → close dialog.

Verify registry:

```bash
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v GamepadMouseSim
```

Expected: entry exists, value is `"<path-to-exe>"`.

- [ ] **Step 3: Uncheck autostart, repeat**

Verify the same `reg query` no longer returns the entry.

- [ ] **Step 4: Edit `config.json` directly to flip the key**

```bash
echo { \"autostart\": false } > build/config.json
```

Wait 1 second (config watcher's 300 ms debounce + 350 ms singleShot = ~700 ms).

Re-check the registry: the entry should be removed within 1 second.

- [ ] **Step 5: No commit — manual evidence only**

If any step fails, fix forward before moving to Task 2.

---

## Task 2: PR2 — Replace magic numbers with named constants

### Task 2.1: Add new constants to `src/core/Types.h`

**Files:**
- Modify: `src/core/Types.h` (alongside `kMaxGamepads`)

- [ ] **Step 1: Add the four constants**

In `src/core/Types.h`, after `constexpr int kMaxGamepads = 4;` (line 82), add:

```cpp
// XInput raw-value normalization ranges (see src/win/XInputWrapper.h).
constexpr int kXInputThumbMax     = 32767;   // signed thumb-stick saturation
constexpr int kXInputTriggerMax   = 255;     // unsigned trigger saturation
constexpr int kXInputDeadzoneMax  = 7849;    // XInput recommended deadzone upper bound
constexpr int kMsPerSecond        = 1000;    // seconds → milliseconds
```

- [ ] **Step 2: Build to confirm header compiles in isolation**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: clean build (no consumers changed yet).

- [ ] **Step 3: Commit**

```bash
git add src/core/Types.h
git commit -m "chore(core): add kXInputThumbMax/TriggerMax/DeadzoneMax/MsPerSecond"
```

### Task 2.2: Replace literals in `GamepadPoller.cpp` and `.h`

**Files:**
- Modify: `src/gamepad/GamepadPoller.cpp:53, 61-71`
- Modify: `src/gamepad/GamepadPoller.h:35`

- [ ] **Step 1: Convert `m_prevState[4]` to `std::array`**

In `src/gamepad/GamepadPoller.h`, replace line 35:

```cpp
    GamepadState m_prevState[4];
```

with:

```cpp
#include <array>
…
    std::array<GamepadState, kMaxGamepads> m_prevState;
```

If `#include <array>` is already present at the top of the header, skip the new include.

- [ ] **Step 2: Replace `32767.0f` in `GamepadPoller.cpp`**

In `src/gamepad/GamepadPoller.cpp`, replace the four occurrences (around lines 61-64):

```cpp
    float lx = xState.Gamepad.sThumbLX / 32767.0f;
    float ly = xState.Gamepad.sThumbLY / 32767.0f;
    float rx = xState.Gamepad.sThumbRX / 32767.0f;
    float ry = xState.Gamepad.sThumbRY / 32767.0f;
```

with:

```cpp
    constexpr float kThumbMax = static_cast<float>(kXInputThumbMax);
    float lx = xState.Gamepad.sThumbLX / kThumbMax;
    float ly = xState.Gamepad.sThumbLY / kThumbMax;
    float rx = xState.Gamepad.sThumbRX / kThumbMax;
    float ry = xState.Gamepad.sThumbRY / kThumbMax;
```

- [ ] **Step 3: Replace `7849.0f / 32767.0f` deadzone expression**

In `src/gamepad/GamepadPoller.cpp`, replace the deadzone line (around line 66):

```cpp
    float deadzone = 7849.0f / 32767.0f;
```

with:

```cpp
    constexpr float kDeadzoneMax = static_cast<float>(kXInputDeadzoneMax);
    float deadzone = kDeadzoneMax / kThumbMax;
```

- [ ] **Step 4: Replace `255.0f` for trigger normalization**

In `src/gamepad/GamepadPoller.cpp`, replace lines 70-71:

```cpp
    state.leftTrigger = xState.Gamepad.bLeftTrigger / 255.0f;
    state.rightTrigger = xState.Gamepad.bRightTrigger / 255.0f;
```

with:

```cpp
    constexpr float kTriggerMax = static_cast<float>(kXInputTriggerMax);
    state.leftTrigger = xState.Gamepad.bLeftTrigger / kTriggerMax;
    state.rightTrigger = xState.Gamepad.bRightTrigger / kTriggerMax;
```

- [ ] **Step 5: Replace the loop bound `4`**

In `src/gamepad/GamepadPoller.cpp`, replace line 53:

```cpp
    for (int i = 0; i < 4; ++i) {
```

with:

```cpp
    for (int i = 0; i < kMaxGamepads; ++i) {
```

- [ ] **Step 6: Build**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: clean build.

- [ ] **Step 7: Verify no literals remain**

Run from `E:/project/sbgj`:

```bash
grep -nE '\b32767\b|\b7849\b|\b255\b' src/gamepad/GamepadPoller.cpp
grep -nE '\b32767\b|\b7849\b|\b255\b' src/gamepad/GamepadPoller.h
grep -nE 'i < 4\b|\[4\]' src/gamepad/GamepadPoller.cpp src/gamepad/GamepadPoller.h
```

Expected: all three greps return no matches.

- [ ] **Step 8: Commit**

```bash
git add src/gamepad/GamepadPoller.h src/gamepad/GamepadPoller.cpp
git commit -m "chore(gamepad): replace XInput literals with named constants; m_prevState[4] -> std::array"
```

### Task 2.3: Replace `* 1000` with `* kMsPerSecond`

**Files:**
- Modify: `src/core/ModeManager.cpp` (one site)
- Modify: `src/core/AutoModeController.cpp` (three sites)

- [ ] **Step 1: Replace in `ModeManager.cpp`**

In `src/core/ModeManager.cpp`, find the line that multiplies the configured seconds by 1000 (audit reference: line 75) and replace `* 1000` with `* kMsPerSecond`.

- [ ] **Step 2: Replace in `AutoModeController.cpp`**

In `src/core/AutoModeController.cpp`, replace all three `m_timer.start(... * 1000);` sites (audit reference: lines 41-42, 51, 64). Use `* kMsPerSecond`.

- [ ] **Step 3: Build**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: clean build.

- [ ] **Step 4: Verify no `* 1000` remain in the targeted files**

```bash
grep -nE '\* *1000\b' src/core/ModeManager.cpp src/core/AutoModeController.cpp
```

Expected: no matches.

- [ ] **Step 5: Commit**

```bash
git add src/core/ModeManager.cpp src/core/AutoModeController.cpp
git commit -m "chore(core): use kMsPerSecond for seconds->ms conversions"
```

### Task 2.4: Smoke verify

- [ ] **Step 1: Launch built exe**

```bash
build/GamepadMouseSim.exe
```

Expected: tray icon; manual_switch lockout still works (long-press LT+View, then immediately try again — second switch should be ignored for ~3 s).

- [ ] **Step 2: Smoke AutoMode timer**

Enable Auto-switch in Settings, set `poll_interval_seconds` to a small value (e.g. 2), click Save. Expected: OSD / logs show periodic polls.

- [ ] **Step 3: No commit — manual evidence only.**

---

## Task 3: PR3 — Tighten log severity

### Task 3.1: Promote `qDebug` to `qWarning` in `Application.cpp`

**Files:**
- Modify: `src/app/Application.cpp:38-39`

- [ ] **Step 1: Replace the warning-shaped log**

Replace lines 38-39:

```cpp
    if (!loaded) {
        qDebug() << "WARNING: No config file found, using defaults";
```

with:

```cpp
    if (!loaded) {
        qWarning() << "No config file found, using defaults";
```

(Note: the closing `}` on line 40 stays where it is — only the body changes.)

- [ ] **Step 2: Commit**

```bash
git add src/app/Application.cpp
git commit -m "chore(log): missing config -> qWarning"
```

### Task 3.2: Promote `qDebug` to `qCritical` in `main.cpp`

**Files:**
- Modify: `src/main.cpp:44-46`

- [ ] **Step 1: Replace the init-failure log**

Replace lines 44-46:

```cpp
    if (!appLogic.initialize()) {
        qDebug() << "Failed to initialize";
        return 1;
    }
```

with:

```cpp
    if (!appLogic.initialize()) {
        qCritical() << "Failed to initialize";
        return 1;
    }
```

- [ ] **Step 2: Build**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: clean build.

- [ ] **Step 3: Verify no `qDebug ... WARNING` strings remain**

```bash
grep -rnE 'qDebug.*WARNING' src/
```

Expected: no matches.

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "chore(log): initialization failure -> qCritical"
```

### Task 3.3: Smoke verify

- [ ] **Step 1: Delete `config.json` and re-launch**

```bash
rm -f build/config.json
build/GamepadMouseSim.exe &
sleep 1
```

Open `debug.log` next to the exe. The "No config file found" line should carry the warning-level prefix as written by `customMessageHandler` in `src/main.cpp:9-18` (the spec does not constrain the exact format; it only requires the level change).

- [ ] **Step 2: No commit — manual evidence only.**

---

## Task 4: PR4 — `.clang-format` + helper scripts + one-shot format

### Task 4.1: Add `.clang-format`

**Files:**
- Create: `.clang-format` (root)

- [ ] **Step 1: Write the file**

Write to `.clang-format`:

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
SortIncludes: true
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
```

- [ ] **Step 2: Commit**

```bash
git add .clang-format
git commit -m "chore(format): add .clang-format"
```

### Task 4.2: Add `.editorconfig`

**Files:**
- Create: `.editorconfig` (root)

- [ ] **Step 1: Write the file**

Write to `.editorconfig`:

```ini
root = true

[*]
indent_style = space
indent_size = 4
end_of_line = lf
insert_final_newline = true
charset = utf-8
trim_trailing_whitespace = true

[*.md]
trim_trailing_whitespace = true
```

- [ ] **Step 2: Commit**

```bash
git add .editorconfig
git commit -m "chore(format): add .editorconfig"
```

### Task 4.3: Add helper scripts

**Files:**
- Create: `scripts/format.ps1`
- Create: `scripts/check-format.ps1`

- [ ] **Step 1: Write `scripts/format.ps1`**

```powershell
# Format every tracked file in-place using clang-format.
# Requires clang-format on PATH.
$ErrorActionPreference = 'Stop'
git ls-files |
    Where-Object { $_ -match '\.(cpp|h|hpp|cc|cxx)$' } |
    ForEach-Object { clang-format -i $_ }
```

- [ ] **Step 2: Write `scripts/check-format.ps1`**

```powershell
# Pre-commit dry-run check. Exits 1 if any tracked file would change.
# Requires clang-format on PATH.
$ErrorActionPreference = 'Stop'
$bad = @()
git ls-files |
    Where-Object { $_ -match '\.(cpp|h|hpp|cc|cxx)$' } |
    ForEach-Object {
        $diff = clang-format --dry-run --Werror $_ 2>&1
        if ($LASTEXITCODE -ne 0) { $bad += $_ }
    }
if ($bad.Count -gt 0) {
    Write-Host "clang-format would change:" -ForegroundColor Red
    $bad | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "clang-format: OK"
```

- [ ] **Step 3: Smoke test on a single file (no commit yet)**

```bash
mkdir -p scripts
# After writing the files, run:
powershell -ExecutionPolicy Bypass -File scripts/check-format.ps1
```

Expected (if PR4 step 4 hasn't run yet): exit code 1 listing source files. That confirms the script works. (You'll do the actual `format -i` sweep in the next sub-task.)

- [ ] **Step 4: Commit scripts**

```bash
git add scripts/format.ps1 scripts/check-format.ps1
git commit -m "chore(format): add scripts/format.ps1 and check-format.ps1"
```

### Task 4.4: Run `clang-format` over `src/` and `tests/`

**Files:**
- Modify: every `*.cpp` / `*.h` / `*.hpp` under `src/` and `tests/`

- [ ] **Step 1: Confirm clang-format is available**

```bash
clang-format --version
```

Expected: prints a version. If not installed, install LLVM for Windows and add to PATH, then retry.

- [ ] **Step 2: Run the formatter**

```bash
powershell -ExecutionPolicy Bypass -File scripts/format.ps1
```

Expected: silently rewrites tracked files in place. (Run from repo root.)

- [ ] **Step 3: Self-check**

```bash
powershell -ExecutionPolicy Bypass -File scripts/check-format.ps1
```

Expected: exit 0, prints "clang-format: OK".

- [ ] **Step 4: Build**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Expected: clean build (formatting is logic-free, but verify).

- [ ] **Step 5: Re-run tests**

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && ctest --test-dir build -R test_config_autostart --output-on-failure'
```

Expected: 4/4 pass.

- [ ] **Step 6: Commit**

```bash
git add -u
git commit -m "chore(format): one-shot clang-format across src/ and tests/"
```

- [ ] **Step 7: Final verification across the whole stack**

Re-run `scripts/check-format.ps1`, the build, and the test suite. All three must pass with no manual fixes required.

---

## Self-Review Checklist (run before handoff)

- [ ] Spec §2 — module boundary: covered by Tasks 1.2, 1.3, 1.4
- [ ] Spec §3.1 — naming style: covered by Task 2.4 (existing naming preserved; PR2 documents that `getNestedValue` rename is deferred)
- [ ] Spec §3.2 — ButtonAction canonical enum: documented in spec; no code change in this plan (no audit violations)
- [ ] Spec §3.3 — naming fixes: covered by Task 2.4 (constants) and Task 1.2 (AutoStart API)
- [ ] Spec §4.1 — log severity tiers: covered by Tasks 3.1, 3.2
- [ ] Spec §4.2 — error handling 4-step: no audit violations to fix; documented for future
- [ ] Spec §4.3 — anti-patterns: covered by Tasks 3.1, 3.2
- [ ] Spec §5.1 — `.clang-format`: Task 4.1
- [ ] Spec §5.2 — `.editorconfig`: Task 4.2
- [ ] Spec §5.3 — helper scripts: Task 4.3
- [ ] Spec §5.4 — soft-enforcement: documented in spec; no CMake/hook additions
- [ ] Spec §6 PR1 — Tasks 1.1 through 1.7
- [ ] Spec §6 PR2 — Tasks 2.1 through 2.4
- [ ] Spec §6 PR3 — Tasks 3.1 through 3.3
- [ ] Spec §6 PR4 — Tasks 4.1 through 4.4
- [ ] Spec §7 verification: each PR has its own smoke / `ctest` / `grep` check
- [ ] Spec §8 test scaffolding: Task 1.6

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-07-13-code-architecture.md`. Two execution options:

1. **Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.
2. **Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints.