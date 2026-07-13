# Code Architecture & Quality Standard — Design Spec

**Date:** 2026-07-13
**Status:** Approved for implementation
**Owner:** GamepadMouseSim maintainers
**Sub-project:** 1 of 5 under the project-standards initiative
  (commit-workflow, user-docs, config-release, testing — separate specs to follow)

---

## 1. Scope & Authority

### 1.1 In scope

C++ / Qt 6.8.3 source under `src/`, plus the top-level `CMakeLists.txt` and the
build-time quality tooling this spec introduces.

Concretely:

- Module dependency direction and UI/subsystem boundaries
- Naming style for classes, methods, members, constants, enums, files
- The canonical `ButtonAction` enum and the steps required to extend it
- Severity-tier policy for `qDebug` / `qInfo` / `qWarning` / `qCritical`
- `.clang-format`, `.editorconfig`, and the accompanying `scripts/*.ps1` wrappers
- A minimal Qt Test scaffolding covering the AutoStart-via-Config regression
- Four companion PRs that bring the existing codebase into compliance

### 1.2 Out of scope (handled by other sub-projects)

| Concern              | Owner sub-project   |
|----------------------|---------------------|
| Commit message format, branch model, PR template | commit-workflow |
| README_CN.md / USER_MANUAL.md update flow, help.png regeneration | user-docs |
| config.json schema versioning, installer output, deploy.ps1 versioning | config-release |
| Broader automated test framework, integration tests, CI hooks | testing |

### 1.3 Authority & conflict resolution

- This spec is the **single source of truth** for the concerns in §1.1.
- `CLAUDE.md` and `AGENTS.md` retain their role as entry-point docs; they may
  summarise or link to this spec but must not duplicate or contradict it.
- When this spec and `CLAUDE.md` / `AGENTS.md` disagree, **this spec wins**
  until the entry-point docs are updated.
- Any future change to §2 – §5 requires a PR that updates both this spec and
  the affected entry-point doc in the same change.
- **Top-level Rule 1 (doc trail)** from `AGENTS.md` applies: any PR that
  changes a public API, a config key, or a subsystem boundary must update
  this spec in the same PR. PRs without a spec delta are rejected at review.
- **Top-level Rule 2 (simulated tests first)** from `AGENTS.md` applies: any
  new behavior lands with an automated `ctest` test in the same PR. The
  §7 verification matrix lists every smoke step; each step must either be a
  `ctest` invocation or an explicit carve-out approved here.

---

## 2. Module Dependency & Boundary Rules

### 2.1 Dependency direction

```
        ui/  ─────────►  input/  ─────────►  gamepad/  ─────────►  win/
                          │                    ▲
                          ▼                    │
                        core/  ◄───────────────┘
                          ▲
                          │
                        app/   (wiring only — no business logic)
```

- Arrows point **from dependent → dependency**. A file may only depend on
  files at its own level or below.
- `app/` is exempt from the directional rule: it is allowed to depend on
  every other module because its sole job is cross-subsystem wiring.

### 2.2 UI is allowed to touch only Config and Types

**Rule.** Any class under `src/ui/` may include `core/Config.h` and
`core/Types.h` and nothing else from `core/`, `input/`, `gamepad/`, or `win/`
(unless explicitly granted an exception by this spec).

**Rule.** All UI-driven state changes go through `Config::setValue`. The
`Config::configChanged` signal — currently parameterless
(`src/core/Config.h:25`, `void configChanged();`) — is the only mechanism
that fans the change out to live subsystems. Subscribers that care which
key changed must compare their cached values against `Config::value(key)`
inside the slot. UI code does not hold pointers to `ModeManager`,
`InputMapper`, `AutoModeController`, `GamepadPoller`, or any other subsystem
beyond the singleton `Config`.

**Exception (legacy, to be retired by PR1).** `AutoStart` currently exposes
public `isEnabled()` / `setEnabled()` and `SettingsDialog` calls them directly.
After PR1 the only writer of the registry is `Application::initialize()`, and
the `SettingsDialog` reads/writes through `Config::setValue("autostart.enabled", ...)`.
No new exceptions will be granted for this kind of bypass.

### 2.3 Config is the single bridge

- All hot-reloadable settings **must** be exposed via `Config::value` /
  `Config::setValue` and live under `<exeDir>/config.json`.
- Subsystems subscribe to `Config::configChanged` in their constructor or
  `Application::initialize()` and re-read their relevant keys.
- No subsystem may poll a file or watch another subsystem for changes.

**Signal semantics — async via the file watcher.** `Config::configChanged`
is **not** emitted synchronously by `Config::setValue`. The precise
contract is documented in §2.3.1. Subscribers must therefore be prepared
for the signal to arrive 300–650 ms after `setValue` returns (300 ms
debounce timer + up to 350 ms Windows watcher re-add singleShot). The
in-memory state seen by `Config::value(key)` is updated **synchronously**
inside `setValue` — only the signal is asynchronous. Subscribers that
need to know the new value must call `Config::value(key)` inside the
slot and compare against their cached value rather than relying on a
signal payload.

### 2.3.1 Config signal contract (precise)

The chain from `setValue` to `configChanged` is:

1. `Config::setValue(key, value)` mutates `m_data` **synchronously**
   (`src/core/Config.cpp:56-62`). `Config::value(key)` returns the new
   value immediately on return. (This is the only observable
   "synchronous" effect of `setValue` from a caller's perspective.)
2. When `m_batchDepth == 0`, `setValue` calls `save()`
   (`src/core/Config.cpp:40-48`), which rewrites `config.json` to disk
   and closes the writer. Closing the file triggers
   `QFileSystemWatcher::fileChanged`.
3. `Config::onFileChanged` (wired at `src/core/Config.cpp:11` and
   `src/core/Config.cpp:35`; body at `src/core/Config.cpp:74-89`):
   - Starts `m_debounceTimer` (single-shot, 300 ms —
     `src/core/Config.cpp:9-10`). On timeout it emits `configChanged`.
   - Schedules a `QTimer::singleShot(350 ms, …)` that re-reads the file
     into `m_data` and re-adds the watched path (Windows watchers drop
     the path once the writer closes the file).
4. Net latency from `setValue` returning to `configChanged` firing on
   the same `Config` instance: 300–650 ms typical; tests should wait
   ~800 ms for headroom against CI jitter.

**Batching.** `beginBatch()` / `endBatch()` defer the file write (and
therefore the signal) until `endBatch()`. If multiple key writes are
made inside a batch, only one `configChanged` fires — after the single
file rewrite at `endBatch()`. The in-memory state, however, reflects
every intermediate `setValue` call as it executes.

**Test pattern.** `QSignalSpy::wait(timeout)` or `QTest::qWait(ms)`.
800 ms is the recommended wait time to span the 300 ms debounce + 350 ms
re-add singleShot plus jitter. Synchronous `QCOMPARE(spy.count(), 1)`
immediately after `setValue` will fail; the test must pump the event
loop first.

### 2.4 Array lengths go through `kMaxGamepads`

**Rule.** Anywhere a compile-time bound on the number of gamepads appears,
the code must reference `kMaxGamepads` (defined in `src/core/Types.h:82`).

Acceptable forms:

```cpp
std::array<ModeManager, kMaxGamepads> m_managers;          // preferred
for (int i = 0; i < kMaxGamepads; ++i) { ... }              // acceptable
```

Unacceptable forms:

```cpp
std::array<ModeManager, 4> m_managers;                     // literal
for (int i = 0; i < 4; ++i) { ... }                        // literal
ModeManager m_managers[4];                                 // literal
```

PR2 retires the two literal-4 sites in `src/gamepad/GamepadPoller.{h,cpp}`.

### 2.5 Wiring lives only in `Application.cpp`

`src/app/Application.cpp::initialize()` is the **only** file allowed to:

- Construct subsystems owned by the application
- Connect signals across subsystems (`connect(src, sig, dst, slot)` where
  `src` and `dst` belong to different subsystems)
- Subscribe to `Config::configChanged` on behalf of subsystems

Subsystems expose their own signals/slots; they never reach into a sibling.

---

## 3. Naming & Enum Conventions

### 3.1 Style table

| Category        | Style                          | Example                              |
|-----------------|--------------------------------|--------------------------------------|
| Class           | `PascalCase`                   | `InputMapper`                        |
| Public method   | `camelCase`, no `get` prefix   | `currentMode()`                      |
| Predicate       | `is` / `has` / `can` prefix    | `isLocked()`, `hasFocus()`           |
| Mutator         | `set` prefix                   | `setMode(Mouse)`                     |
| Private method  | `camelCase`, same as public    | `releaseModifiers()`                 |
| Member variable | `m_` + `camelCase`             | `m_ltTabBlocked`                     |
| Constant        | `k` + `PascalCase`             | `kMaxGamepads`, `kGamepadPollIntervalMs` |
| Enum value      | `PascalCase` (no `k` prefix)   | `ButtonAction::AltTab`               |
| File            | Matches primary class          | `InputMapper.h`, `InputMapper.cpp`   |
| Namespace       | None — the project uses none   | —                                    |

**Exception.** `Config::getNestedValue` is a private helper
(`src/core/Config.h:31`). Private helpers are exempt from the public-API
naming rule, but the public-facing pattern still applies to any future
public accessor. PR2 may rename it for consistency, but it is **not**
required; the rule's force is reserved for the public API surface.

### 3.2 `ButtonAction` is canonical

**Rule.** Any mapping from a gamepad button to a key/command must use the
`ButtonAction` enum (`src/core/Types.h:11`) and its three companion functions:

- `stringToAction(const QString&)` — `src/core/Types.cpp:3`
- `actionToString(ButtonAction)` — `src/core/Types.cpp:53`
- `actionToChinese(ButtonAction)` — `src/core/Types.cpp:105`

These three functions power:

- The `config.json` button→action mapping (`stringToAction` on read,
  `actionToString` on write via `SettingsDialog`)
- The fullscreen Chinese help overlay (`actionToChinese`)
- The settings dialog combo-box labels (`actionToString` + `actionToChinese`)

**5-step checklist for adding a new `ButtonAction` value:**

1. Append the enum value in `ButtonAction` in `src/core/Types.h`.
2. Add the matching string in `actionToString` / `stringToAction` in
   `src/core/Types.cpp`.
3. Add the Chinese label in `actionToChinese` in `src/core/Types.cpp`.
4. If the action has a modifier-layer variant (LT / RT), update
   `KeyboardMapper::resolveModifierAction` accordingly.
5. Re-run `tools/generate_help_png.py` so `resources/icons/help.png` is
   regenerated; commit the regenerated PNG.

Skipping any step will cause at least one of: settings-dialog crashes
(empty string in `stringToAction`), silent config corruption, or stale help
text.

### 3.3 Naming fixes bundled with PR2

| Current                                              | New                                          | File                  |
|------------------------------------------------------|----------------------------------------------|-----------------------|
| `Config::getNestedValue`                             | *(deferred — private helper; rename only if no API churn)* | `src/core/Config.h`   |
| Literal `32767.0f` ×4                                | `kXInputThumbMax`                            | `src/core/Types.h`    |
| Literal `7849.0f`                                    | `kXInputDeadzoneMax`                         | `src/core/Types.h`    |
| Literal `255.0f` ×2                                  | `kXInputTriggerMax`                          | `src/core/Types.h`    |
| Literal `1000` (seconds→ms) ×4                       | `kMsPerSecond`                               | `src/core/Types.h`    |
| `GamepadPoller::m_prevState[4]`                      | `std::array<GamepadState, kMaxGamepads>`     | `src/gamepad/GamepadPoller.h` |
| `GamepadPoller.cpp` loop bound `4`                   | `kMaxGamepads`                               | `src/gamepad/GamepadPoller.cpp` |

All four new constants land in `src/core/Types.h` (the project already treats
this header as the home of cross-cutting constants).

---

## 4. Error Handling & Logging

### 4.1 Severity tiers

| Call          | Use for                                                                                                  |
|---------------|----------------------------------------------------------------------------------------------------------|
| `qDebug()`    | Lifecycle, state changes, input activity, diagnostics. Visible in `debug.log` next to the exe.           |
| `qInfo()`     | User-perceptible normal events (e.g. config auto-reloaded).                                              |
| `qWarning()`  | **Recoverable anomalies** — missing optional resource, config fallback, user-config error.               |
| `qCritical()` | **Unrecoverable for the affected subsystem** — XInput unavailable, key resource creation failed.          |
| `qFatal()`    | **Forbidden.** Do not call. Use `qCritical()` + explicit `return` instead.                               |

Current usage as audited on 2026-07-13: 44 `qDebug`, 1 `qWarning`, 0 `qInfo`,
0 `qCritical`. The imbalance is fixed by PR3.

### 4.2 Four-step error handling

Every error branch must do all four, in order:

1. **Log** at the appropriate severity (§4.1). No silent `return false;`.
2. **Decide user notification.** Use OSD popup, tray-icon tooltip, or both —
   never the log alone.
3. **Pick a fallback.** Default values, last-known-good config, or skip the
   feature. Document the fallback in the log line.
4. **Update health state.** If the subsystem has a `healthy()` getter, set
   it. Subsystems without one must add one when they grow past one optional
   dependency.

### 4.3 Anti-patterns

- ❌ Embedding `"WARNING:"` (or any severity marker) in a `qDebug()` string.
  Audit hit: `src/app/Application.cpp:38-39`. PR3 converts it to `qWarning`.
- ❌ Using `qDebug` for an explicit failure branch (`src/main.cpp:44-46`).
  PR3 converts it to `qCritical`.
- ❌ Logging full exception messages with stack traces to OSD — keep the log
  verbose and the OSD short.

---

## 5. Code Quality Mechanism

### 5.1 `.clang-format` (new, root of repo)

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

### 5.2 `.editorconfig` (new, root of repo)

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

### 5.3 Helper scripts (new, `scripts/`)

- `scripts/format.ps1` — formats all tracked files via
  `git ls-files | clang-format -i`.
- `scripts/check-format.ps1` — runs `clang-format --dry-run --Werror` over
  `git diff --cached --name-only`. Exit code 1 on any diff.

### 5.4 Soft-enforcement model

- **No** pre-commit hook (avoids the Git-Bash-on-Windows hook setup cost).
- **No** CMake integration (does not block build for users without
  clang-format installed).
- **No** CI (added by the testing sub-project).
- Documentation in `CLAUDE.md` and `CONTRIBUTING.md` instructs contributors
  to run `scripts/check-format.ps1` before pushing.

### 5.5 What this spec does NOT introduce

- ❌ `clang-tidy` — adds a separate LLVM tool dependency on Windows; project
  size does not justify the maintenance burden.
- ❌ `pre-commit` framework — adds a Python dependency the project does not
  otherwise need.
- ❌ `cppcheck` — same justification as clang-tidy; revisit under the
  testing sub-project if a CI host becomes available.

---

## 6. Companion PRs

Four PRs, applied in order. Each PR stands alone but builds on the previous
one.

### PR1 — `fix: bring AutoStart into the Config hot-reload flow`

**Key name decision.** The codebase already uses a top-level key
`"autostart"` (a boolean), read at `src/app/Application.cpp:115` and
written at `src/ui/SettingsDialog.cpp:426`. **PR1 keeps the existing key
name and shape** (top-level `"autostart": bool) so existing `config.json`
files continue to work without a migration. Renaming to `"autostart.enabled"`
would be a breaking schema change and falls under the config-release
sub-project.

**Files touched:**

- `src/core/AutoStart.{h,cpp}` — remove public `isEnabled` /
  `setEnabled`. Replace with a single static helper:
  ```cpp
  // AutoStart.h
  static void applyToRegistry(bool enabled);
  ```
  Body is the current `setEnabled` body (`src/core/AutoStart.cpp:21-35`).
- `src/ui/SettingsDialog.cpp` — drop `#include "core/AutoStart.h"`. Replace
  `AutoStart::isEnabled()` (line 354) with
  `m_config->value("autostart", false).toBool()`. Replace the
  `if (as != AutoStart::isEnabled()) AutoStart::setEnabled(as);` block
  (lines 422-424) with **no code at all** — the line below it already
  writes `m_config->setValue("autostart", as);` which triggers the
  configChanged signal that `Application` listens to.
- `src/app/Application.cpp` — replace the `if (m_config.value("autostart", false).toBool()) { ... }` block (lines 115-123) with a single call:
  ```cpp
  AutoStart::applyToRegistry(m_config.value("autostart", false).toBool());
  ```
  Augment the existing `connect(&m_config, &Config::configChanged, ...)`
  block (lines 106-112) so the lambda also reads the `autostart` key and
  calls `AutoStart::applyToRegistry`. Caching the last value inside
  Application prevents redundant registry writes when unrelated keys
  change.
- `tests/CMakeLists.txt`, `tests/test_main.cpp`,
  `tests/test_config_autostart.cpp` — added (see §8).
- `config/default_config.json` — add `"autostart": false` so first-run
  users get an explicit key. The existing `Config::load` fallback
  (`src/core/Config.cpp:19-25`) already writes `default_config.json` to
  `<exeDir>/config.json` on first run.
- Commit message type: `fix`.

### PR2 — `chore: replace magic numbers with named constants`

- `src/core/Types.h` — add `kXInputThumbMax`, `kXInputTriggerMax`,
  `kXInputDeadzoneMax`, `kMsPerSecond` (`constexpr`).
- `src/gamepad/GamepadPoller.cpp` — replace literals; `for` bound becomes
  `kMaxGamepads`.
- `src/gamepad/GamepadPoller.h` — `GamepadState m_prevState[4]` →
  `std::array<GamepadState, kMaxGamepads> m_prevState`.
- `src/core/ModeManager.cpp`, `src/core/AutoModeController.cpp` — replace
  `* 1000` with `* kMsPerSecond`.
- `src/core/Config.{h,cpp}` — rename `getNestedValue` → `nestedValue`.
- Commit message type: `chore`.

### PR3 — `chore: tighten log severity levels`

- `src/app/Application.cpp:38-39` — `qDebug` → `qWarning`; drop the
  `"WARNING:"` substring.
- `src/main.cpp:44-46` — `qDebug` → `qCritical`.
- Commit message type: `chore`.

### PR4 — `chore: format codebase per .clang-format`

- One-time `clang-format -i` across `src/`, `tests/`, `installer/`.
- `.clang-format` + `.editorconfig` + `scripts/*.ps1` added in the same PR.
- Pure-formatting commit; reviewers should focus on diff size, not logic.
- Commit message type: `chore`.

### PR ordering rationale

PR1 must land before PR2 because PR2 touches files PR1 also touches. PR3
touches lines PR1 / PR2 may edit, so it follows. PR4 last so the large
format-only diff is isolated from any logic changes.

---

## 7. Verification

### 7.0 Verification philosophy (per AGENTS.md Rule 2)

Every verification step is either an automated `ctest` invocation or an
explicitly carved-out manual smoke. Per AGENTS.md, a manual-smoke carve-out
is allowed only when the behavior cannot be simulated AND the spec names the
carve-out. The carve-outs in this spec are tracked in §7.4 with owners and
follow-up issue IDs.

### 7.1 Automated verification (every PR)

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release ^
   && ctest --test-dir build --output-on-failure'
```

Pass criteria: all `ctest` cases pass; zero new warnings vs baseline commit
`7a58792`.

### 7.2 Manual smoke (only the explicitly carved-out steps)

1. Launch the built `GamepadMouseSim.exe`; system-tray icon appears.
   *(carved out: visual tray rendering — see §7.4)*
2. Long-press LT + View for 1 second; mode flips and OSD shows.
   *(carved out: real gamepad + visual OSD — see §7.4)*
3. Press LT + R3; fullscreen help overlay appears in Chinese.
   *(carved out: real gamepad + fullscreen rendering — see §7.4)*
4. Open Settings dialog; the autostart checkbox reflects the registry state.
   *(carved out: Qt Widgets rendering — see §7.4)*

### 7.2 PR-specific verification

| PR  | How to verify                                                                                   | Pass criteria                          |
|-----|--------------------------------------------------------------------------------------------------|----------------------------------------|
| PR1 | Toggle autostart in Settings dialog; check `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`. | Both directions work; survives restart |
| PR1 | `ctest -R test_config_autostart`                                                                | 4 cases pass                            |
| PR2 | `grep -nE '\b32767\b\|\b7849\b\|\b255\b' src/` returns only definitions in `Types.h`            | No call-site literals                  |
| PR2 | `grep -nE 'i < 4\b\|\[4\]' src/gamepad/`                                                        | Empty                                  |
| PR3 | `grep -nE 'qDebug.*WARNING' src/`                                                                | Empty                                  |
| PR4 | `scripts/check-format.ps1`                                                                       | Exit 0                                  |

### 7.3 Build verification

```bash
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

Pass criteria: zero new warnings compared to baseline commit
`7a58792`; zero compile errors.

### 7.4 Manual-smoke carve-outs (per AGENTS.md Rule 2)

| Step | Reason it cannot be simulated today | Owner | Follow-up |
|------|-------------------------------------|-------|-----------|
| §7.2.1 Tray icon visible | Qt `QSystemTrayIcon` rendering requires a live Windows shell session; no headless harness. | — | Issue TBD; revisit under testing sub-project. |
| §7.2.2 LT+View → mode flip | Requires real `XInputGetState` reports from a connected gamepad; XInput has no public test double. | — | Issue TBD; consider a fake `IXInput` shim. |
| §7.2.3 LT+R3 → help overlay | Same as §7.2.2 + visual rendering. | — | Issue TBD. |
| §7.2.4 Autostart checkbox state | Settings dialog requires a live user click; UI behavior not yet unit-testable without a UI test framework. | — | Issue TBD; revisit when Squish/pytest-qt is evaluated. |

Until each row above is closed, the corresponding manual-smoke step is the
acceptance gate for that behavior, and the PR description must state
"manual-smoke verified" with the verifier's initials and date.

---

## 8. Test Scaffolding (PR1 deliverable)

### 8.1 Files

```
tests/
├── CMakeLists.txt
├── test_main.cpp
└── test_config_autostart.cpp
```

### 8.2 `tests/CMakeLists.txt`

```cmake
find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(test_config_autostart
    test_main.cpp
    test_config_autostart.cpp
)
target_link_libraries(test_config_autostart PRIVATE
    Qt6::Test
    Qt6::Core
    src_core              # see §8.3
)
add_test(NAME test_config_autostart COMMAND test_config_autostart)
```

### 8.3 Library split

To make `Config` linkable into the test binary, `CMakeLists.txt` is updated
to expose `src/core/` as a static library target `src_core`:

```cmake
add_library(src_core STATIC
    src/core/Config.cpp
    src/core/Types.cpp
)
target_include_directories(src_core PUBLIC src)
target_link_libraries(src_core PUBLIC Qt6::Core)
```

The main `GamepadMouseSim` target then links `src_core` (and continues to
link the rest of `src/` via its existing `target_sources` block). This is a
no-op for production builds other than enabling test linkage.

### 8.4 Test cases

`Config::configChanged` is parameterless (see §2.2). Tests therefore use
a `QSignalSpy` and assert on signal-count + post-slot value reads, not on
a payload.

`Config::onFileChanged` triggers reload via a 350 ms `QTimer::singleShot`
followed by a 300 ms `m_debounceTimer` (see `src/core/Config.cpp:74-89`).
Tests must wait **≥ 700 ms** to be robust against scheduling jitter; the
test code uses `QTest::qWait(800)`.

**Why the wait.** `Config::configChanged` is **not** emitted synchronously
by `Config::setValue`. `setValue` mutates `m_data` synchronously (so
`value("autostart")` returns the new value immediately on return), but
the `configChanged` signal is driven by the `QFileSystemWatcher` chain
that observes the file write — see §2.3.1 for the precise contract.
Asserting `QCOMPARE(spy.count(), 1)` immediately after `setValue` will
fail; the test must pump the event loop first via `QTest::qWait(800)`.

| Test case                                      | Assertion                                                     |
|------------------------------------------------|---------------------------------------------------------------|
| `setValue_emitsConfigChanged`                  | After `setValue("autostart", true)` and `QTest::qWait(800)`, `configChanged` fires exactly once; subsequent `value("autostart", false)` returns `true`. (Note: `value("autostart")` returns the new value immediately because `m_data` is mutated synchronously — only the signal is async.) |
| `setValue_writesToDisk`                        | After `setValue("autostart", true)`, re-reading the on-disk JSON file yields `"autostart": true`. |
| `reloadReReadsValue`                           | External write to `config.json` + `QTest::qWait(800)` → `configChanged` fires; `value("autostart")` returns the externally written value. |
| `nestedPathRoundTrip`                          | `setValue("a.b.c", 42)` + `value("a.b.c")` round-trips; sibling keys (e.g. `"a.x"`) are untouched. |

### 8.5 Out of scope for these tests (and how to lift the carve-out)

| Item                                          | Why carved out                                                       | Path to simulate                                                                                  |
|-----------------------------------------------|----------------------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| Full registry round-trip via `AutoStart`      | `QSettings(NativeFormat)` writes to HKCU, requires Windows user perms. | Refactor `AutoStart` to take an injected `IRegistry` interface; tests use an in-memory fake. (Tracked under the testing sub-project; not in PR1.) |
| Subsystem reactions to `configChanged`        | Live subsystems (ModeManager, InputMapper) require a Qt event loop and live XInput. | Inject a fake `IConfigSubscriber` and assert callbacks fire; deferred to testing sub-project.    |
| Log severity                                  | No public assertion surface on `qInstallMessageHandler`.             | Capture `QtMsgHandler` output into a string buffer in the test; assert substring + level. (Tracked under testing sub-project; PR3 adds the capture seam.) |
| LT+View hold → mode flip                      | Requires a real gamepad input stream.                                | Inject fake `IXInput` shim returning scripted `GamepadState`s; **planned for PR1's follow-up**. |
| Help overlay rendering                        | Requires live Qt Widgets rendering surface.                          | `QWidget::grab()` + image diff; deferred.                                                         |

For PR1 specifically, the `setValue_*` and `reloadReReadsValue` tests in §8.4
**are** simulated: they use `QTemporaryFile` for the on-disk JSON and avoid
real HKCU writes. This satisfies AGENTS.md Rule 2 for PR1's behavior surface.

---

## Appendix A — Cross-references to sibling sub-projects

- **Commit & Workflow** (next): will define Conventional Commits prefixes
  (`fix` / `chore` / `feat`) used in §6. Will also define the PR template
  that references the verification matrix in §7.
- **User Docs**: will own the `CONTRIBUTING.md` introduction that links to
  `scripts/check-format.ps1` (§5.3) and to the manual smoke steps in §7.1.
- **Config & Release**: will own the schema-versioning policy that determines
  whether `config/default_config.json` additions in PR1 are a breaking
  change.
- **Testing**: will own the path from this minimal `tests/` scaffold to a
  full test suite + CI hook.