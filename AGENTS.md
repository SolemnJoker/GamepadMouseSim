# AGENTS.md

Compact repo-specific notes for OpenCode sessions. Full architecture lives in `CLAUDE.md`; Chinese user-facing docs are in `README.md`. Read both before non-trivial changes.

## Project at a glance

- **GamepadMouseSim** — Windows desktop app. Qt 6.8.3 (Widgets + Svg) + C++17 + CMake. XInput, up to 4 gamepads.
- One executable, no main window; runs from the system tray. Logs to `debug.log` next to the exe and to OutputDebugString.
- Output is `WIN32_EXECUTABLE` (no console window). Debug output still goes to `debug.log` + OutputDebugString via the custom `qInstallMessageHandler` in `src/main.cpp`.

## Build

Qt is discovered via `find_package(Qt6 ...)` — no hardcoded install path. Pass
`-DCMAKE_PREFIX_PATH="C:/Qt/<version>/<kit>"` at configure time (or set the
`CMAKE_PREFIX_PATH` environment variable). The existing `build/` cache already
knows the local path, so incremental builds need nothing extra.

```bash
# MSVC + Ninja (from repo root)
cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j 8

# Or MSVC multi-config (matches deploy.ps1):
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'
```

The help overlay is rendered at runtime from the live config (no build-time image; `tools/generate_help_png.py` was retired 2026-09-13).

## Package

```bash
"C:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe" --no-translations build/GamepadMouseSim.exe
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\GamepadMouseSim.iss
powershell -File skills/deploy/deploy.ps1 -TargetDir "E:\program files\GamepadMouseSim"
```

`deploy.ps1` references fixed paths (VS 2022 Community, Qt 6.8.3). Edit lines 9–11 if your install differs. `deploy.ps1` never overwrites user data in the target dir (`config.json`, `debug.log`, `config/` are excluded from the copy). The tray menu offers "恢复默认配置" (factory reset with confirmation dialog → `Config::restoreFactoryDefaults`).

## Architecture essentials

- **Single wiring file**: `src/app/Application.cpp`. Every signal/slot across subsystems lives there. Read it first when changing component interactions.
- **`kMaxGamepads = 4`** in `src/core/Types.h:82`. Subsystems that touch pads hold `std::array<T*, kMaxGamepads>` and route by index — never runtime lookup.
- **Two threads total**: the Qt event loop + a private `QThread` in `src/gamepad/GamepadPoller.cpp` that polls all 4 controllers at 60 Hz (~16 ms, see `kGamepadPollIntervalMs`).
- **`processTrigger` runs in both modes** (so LT/RT modifier state is tracked everywhere); **`processButton` only in Mouse mode**. Default mode still handles R3 (help) and forwards LT+View to the combo detector — see `InputMapper.cpp`.
- **Mode switch releases held modifiers** via `InputMapper::releaseModifiers()` triggered by `ModeManager::setMode`, so Alt/Ctrl can't get stuck.
- **Auto-switch is one-way**: `AutoModeController` only flips Mouse→Default on game detection; it never auto-flips back. Boot default is Default.

## Config

`src/core/Config.cpp` wraps a `QJsonObject` with dot-path lookup and a `QFileSystemWatcher` + 300 ms debounce for hot reload. Settings GUI writes through `Config::setValue` and the `configChanged` signal fans out to all `ModeManager` / `InputMapper` / `AutoModeController` — there is no separate apply path. **Use hot reload; do not poke subsystems directly from `SettingsDialog`.**

Bootstrap order in `Application::initialize()`:
1. `<exeDir>/config.json`
2. `<exeDir>/config/default_config.json` (bundled via `resources/resources.qrc`); auto-writes a fresh writable `config.json` on first use.
3. Hard-coded defaults.

## Conventions

- **`ButtonAction` enum is canonical** for button→key mapping. Use `stringToAction` / `actionToString` / `actionToChinese` in `core/Types.cpp`. The settings dialog and help overlay both read from it.
- **Sticky modifier actions** (e.g. Alt+Tab) need button-level debounce flags in `KeyboardMapper` — see existing `m_ltTabBlocked`. Add new flags there when adding similar actions, otherwise Alt can release prematurely.
- **Game-detection sustained thresholds** use accumulator windows in `GameDetector` (`m_cpuSustainedAccumMs` etc.) — instant spikes must not auto-switch modes.
- **Win32 autostart** writes to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` (no admin), via `src/core/AutoStart.{h,cpp}`. Driven by `autostart` config key.
- **Commit format**: see `docs/superpowers/specs/2026-07-14-commit-workflow-design.md` §2. Use `fix:`, `feat:`, `refactor:`, `test:`, `chore:`, `docs:`, `build:`, `style:` with an optional module-scope and a concise imperative description.
- **Merge checklist**: see `PULL_REQUEST_TEMPLATE.md` in the repo root. Every PR or merge-to-master should verify each line before proceeding.
- **Branch naming** (recommended): `<type>/<kebab-case-description>`. Direct master pushes are still allowed.
- `.gitignore` excludes `build/`, `dist/`, `*.exe`, `*.dll`, `*.log`, generated `moc_*`/`qrc_*`, `GamepadMouseSim_autogen/`, `.mimocode/`, `review/`, `skills/`, `resources/icons/help.png`, `nul`. (`docs/` was unignored 2026-07-13 to host standards/specs.)

## Top-level project rules (apply to every change)

These rules override lower-level guidance when they conflict. They are also
the criteria used by code review to accept or reject a change.

### Rule 1 — Design and key changes must be documented

Every **design change** or **key behavior change** ships with a doc update in
the same PR (or, for batched work, in a doc-only commit that lands before the
behavior commit). Specifically:

- A new subsystem, a new public API, or any change to the wiring in
  `src/app/Application.cpp` → update `docs/superpowers/specs/<date>-*.md` and
  the architecture diagram in `CLAUDE.md`.
- A new `ButtonAction` value or any change to the button→key mapping → update
  `README.md` (run `scripts/sync_docs.py` to refresh the button table; the
  help overlay regenerates automatically at runtime from
  `src/core/MappingDefaults`).
- A config-schema change (new key, removed key, default change) → update
  `config/default_config.json` and add an entry to the migration notes in
  `docs/superpowers/specs/config-release-design.md` (when that spec lands).
- Any change to the build, packaging, or deploy steps → update `CLAUDE.md`
  "Build & Run" / "Package" sections.
- The PR description must link to the spec/plan file path that captures the
  change. PRs without a doc trail are rejected at review.

### Rule 2 — Features must have automated tests; user-driven flows must be simulated first

Every feature ships with at least one automated test that runs via `ctest`.
Tests live under `tests/` and use Qt Test (no other framework without an
explicit exception in the spec).

User-driven flows that cannot be exercised without a real gamepad, a real
registry write, a real fullscreen app, or real human input must be
**simulated first** before they can ship as "manual smoke only". A simulated
test:

- Stubs or fakes the real-world collaborator (gamepad state, HKCU writes,
  foreground-window queries, etc.) so the test runs in CI without privileges
  or hardware.
- Covers the happy path and the documented error/edge branches.
- Lives in `tests/` alongside the unit tests; the spec/plan notes any
  unresolved branches still requiring manual smoke.

A test is "simulated first" if it lands in the same PR as the behavior it
covers. Manual-smoke-only steps in a plan are acceptable only when:

- The behavior genuinely cannot be simulated (e.g. visual rendering of the
  OSD overlay, system-tray icon swap), AND
- The spec explicitly approves the manual-smoke carve-out, AND
- A follow-up issue is filed to convert the smoke step to a simulated test.

These rules live in `docs/superpowers/specs/` (the per-area specs refine
them) and in the project-standards initiative spec family.