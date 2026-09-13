# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**GamepadMouseSim** — Windows desktop app that maps Xbox gamepads (XInput, up to 4 simultaneous) to mouse/keyboard input. Backed by Qt 6.8.3 (Widgets + Svg) on top of a C++17/CMake project. Ships an Inno Setup installer and an optional PowerShell deploy script.

Primary usage:
- Long-hold **LT+View for 1s** to toggle Mouse↔Default mode per pad.
- **LT + (button)** enters an Alt-layer combo (e.g. LT+X → Alt+Tab). **RT + (button)** enters a Ctrl-layer.
- **LT+R3** shows a fullscreen Chinese help overlay.
- Tray icon swaps between mouse/gamepad SVG based on mode; OSD popup on mode change.

Full input reference lives in `README.md` (Chinese). This file only covers architecture + dev workflow.

## Build & Run

CMakeLists hardcodes the Qt path to `C:/Qt/6.8.3/msvc2022_64` and links `xinput` + `dxgi` (for GPU usage). Reconfigure if Qt is elsewhere.

```bash
# Configure + build (MSVC)
cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j 8

# Or use the full MSVC release path (matches deploy.ps1):
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 ^
   && cmake --build build --config Release'

# Bundle Qt DLLs next to the .exe
"C:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe" --no-translations GamepadMouseSim.exe

# Build installer
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\GamepadMouseSim.iss
# -> installer\GamepadMouseSim-setup.exe

# End-to-end deploy (build + windeployqt + copy to target dir)
powershell -File skills/deploy/deploy.ps1 -TargetDir "E:\program files\GamepadMouseSim"
```

The help overlay renders at **runtime** from the live config (`src/ui/HelpContent` + `OsdOverlay`); there is no build-time help image anymore. The app also supports `GamepadMouseSim.exe --selftest`: a process-level config-chain self-check (qrc resource → load chain → migration → profile expansion → merged view) that prints `[PASS]/[FAIL]` per item and exits 0/1 — run directly by ctest as `selftest_config`.

The resulting binary is `WIN32_EXECUTABLE` (no console window). Log output
is written to `debug.log` next to the exe and to `OutputDebugString` via a
custom `qInstallMessageHandler` in `src/main.cpp`.

## High-Level Architecture

```
                       main.cpp
                          │
                          ▼
                     Application  (src/app)
                      (wiring only)
                          │
        ┌────────────┬────┴────┬────────────┐
        ▼            ▼         ▼            ▼
    Config      GamepadPoller  SystemTray  OsdOverlay     (singletons)
   (JSON+hot
   reload)
                         │ gamepadStateChanged(idx, state)
                         ▼
   per pad (4×, one set of each):
   ComboKeyDetector[idx] ──► ModeManager[idx] ──► InputMapper[idx]
                                                    │
                                                    ├─► MouseMapper
                                                    └─► KeyboardMapper
                                                          (LT / RT
                                                          modifier layers)
                         ▲
                         │
                   AutoModeController  →  GameDetector  →  ProcessDetector
                                                         →  SysUsage (CPU+GPU)
                                                         →  isFullscreenForeground()

   virtual keyboard (shared, one instance):
   KeyboardController  ──►  KeyboardNavController  ──►  KeyInjector (SendInput)
        │  (bypasses InputMapper while overlay is open)        ▲
        └──►  KeyboardOverlay (frameless, no-activate)         └── input/ level

   config & help (2026-09-13):
   Config  ── load chain: exe-side config.json → qrc :/config/default_config.json
           │                → MappingDefaults skeleton; auto-writes writable config.json
           ├─ profiles.{active,order,list} expanded to mouse_mode.* (runtime reads
           │  mouse_mode.* only); GUI saves mirror back to the active profile
           ├─ HelpContent(config) ──► OsdOverlay help screen (runtime-rendered,
           │  no build-time image)
           └─ MappingDefaults = single source of default mappings (runtime fallback,
              settings GUI merged view, help content, migration backfill)
```

**Single-source-of-truth for cross-component wiring** is `Application::initialize()` in `src/app/Application.cpp`. Read that file first whenever changing subsystem interactions.

### Key invariants

- **`kMaxGamepads = 4` constant** in `src/core/Types.h`. Every subsystem that touches pads holds a `std::array<T*, kMaxGamepads>` so signals carrying `controllerIndex` (0–3) can be routed by indexing — never by runtime lookup.
- **Per-pad state is fully independent**: each pad has its own `ComboKeyDetector`, `ModeManager`, and `InputMapper`. The tray/tray menu and the auto-mode controller iterate all 4.
- **Two threads total**: main Qt event loop owns everything, and `GamepadPoller::PollThread` (a private `QThread` subclass in `src/gamepad/GamepadPoller.cpp`) loops `XInputGetState(0..3)` at **60 Hz** (~16 ms) and emits `gamepadStateChanged` via queued connection.
- **`processTrigger` is called in *both* modes** (so LT/RT modifier state tracking isn't dropped); `processButton` is only called in Mouse mode. Default-mode `InputMapper` still handles R3 (help) and forwards LT+View to the combo detector.
- **Modifier release on mode switch**: `ModeManager::setMode` triggers `InputMapper::releaseModifiers()` so Alt/Ctrl can't get "stuck" if held when switching.
- **`KeyboardMapper` blocks the LT+View combo from also emitting Alt+Tab** — this is intentional and lives in modifier-action resolution.
- **Auto-switch direction is one-way**: `AutoModeController` only flips Mouse→Default when a game is detected (game list / fullscreen / sustained CPU% / sustained GPU%). It never auto-switches back. Boot defaults to Default.
- **Virtual keyboard input bypass** (`src/input/KeyboardController.{h,cpp}`): while the keyboard overlay is open, raw `GamepadState` goes to `KeyboardController` (navigation + close detection) and **not** to `InputMapper`; the mode-switch combo stays alive via `ComboKeyDetector`, which is fed first. Open detection is `InputMapper`'s `ShowKeyboard` action (default binding L3-held layer + Menu, i.e. the config `LT` layer — entered by holding L3, left-stick click); close detection lives in `KeyboardController`, which reverse-looks-up the current `ShowKeyboard` binding from config. The overlay is `WS_EX_NOACTIVATE` (Qt::WindowDoesNotAcceptFocus) so the foreground window never loses focus — injected keys land in the window the user was typing in. Any pad switching to Default mode closes the keyboard.
- **Keyboard injection channel discipline** (`src/input/KeyInjector.h`): B1 uses `sendVk()` (real VK events so the target's IME composes normally); the `commitText()` UNICODE-channel method is reserved for a future embedded pinyin engine and must not be used for letters in B1 — UNICODE injection bypasses IME composition entirely.
- **Boot autostart** uses `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` (no admin), set/cleared in `Application::initialize()` from `autostart` config key. Wrapped in `src/core/AutoStart.{h,cpp}`.

### Signal/slot shape (the routes set up in `Application::initialize()`)

| Source                                           | Sink                                                                            |
|--------------------------------------------------|---------------------------------------------------------------------------------|
| `GamepadPoller::gamepadStateChanged(idx, state)` | `ComboKeyDetector[idx].onGamepadState`, then `KeyboardController.onGamepadState` (if consumed — keyboard open — the InputMapper call is skipped), else `InputMapper[idx].onGamepadStateChanged` |
| `ComboKeyDetector::comboTriggered(idx)`          | `ModeManager[idx].manualSwitch`                                                 |
| `ModeManager::modeChanged(idx, mode)`            | `InputMapper[idx].onModeChanged`, `SystemTray::onModeChanged`, `OsdOverlay::showModeChange`, `KeyboardController.onModeChanged` |
| `InputMapper::showHelpRequested`                 | `OsdOverlay::showHelp`                                                          |
| `InputMapper::showKeyboardRequested`             | `KeyboardController.openOverlay`                                                |
| `SystemTray::switchModeRequested / lockModeRequested / pauseRequested` | each → iterate all 4 `ModeManager`s                           |
| `Config::configChanged`                          | every `ModeManager`, every `InputMapper`, `AutoModeController` (so settings GUI applies live without restart) |

### Config system

`src/core/Config.{h,cpp}` wraps a `QJsonObject` and exposes dot-path lookups (`m_config->value("left_stick.sensitivity_x", 1.0).toDouble()`). Backed by a `QFileSystemWatcher` + 300 ms debounce `QTimer` for hot reload. Paths are dot-separated key segments; set with `setValue("a.b.c", val)`.

First-run bootstrap in `Application::initialize()`:
1. Try `<exeDir>/config.json`.
2. Fall back to `<exeDir>/config/default_config.json` (bundled via `resources/resources.qrc`); on that fallback it auto-writes a fresh `config.json` so the user gets a writable copy.
3. If neither loads, the app continues with hard-coded defaults.

`SettingsDialog` (tabbed GUI in `src/ui/`) writes back through `Config::setValue`, which fans out via the same `configChanged` signal everything else listens to — no separate apply path.

### Where things live

| Concern                      | File                                       |
|------------------------------|--------------------------------------------|
| Entry + log file             | `src/main.cpp`                             |
| Wiring / orchestration        | `src/app/Application.cpp`                  |
| Enums + button-bit constants | `src/core/Types.h` (`ButtonAction`, `GamepadMode`, `GamepadState`, XInput bit flags) |
| Config (JSON, hot reload)    | `src/core/Config.{h,cpp}`                  |
| Mode state machine (per pad) | `src/core/ModeManager.{h,cpp}`             |
| Optional auto-switch         | `src/core/AutoModeController.{h,cpp}` + `GameDetector.{h,cpp}` |
| Game-detection sources       | `core/ProcessDetector`, `core/SysUsage` (CPU+GPU via D3DKMT), fullscreen-foreground helper in `GameDetector.cpp` |
| Win32 autostart registry     | `src/core/AutoStart.{h,cpp}`               |
| 60 Hz XInput loop            | `src/gamepad/GamepadPoller.cpp`            |
| LT+View hold detector        | `src/gamepad/ComboKeyDetector.cpp`         |
| Mode dispatch + help signal  | `src/input/InputMapper.cpp`                |
| Left stick → mouse, right → scroll | `src/input/MouseMapper.cpp`           |
| Buttons → keys, modifier layers, help text generation | `src/input/KeyboardMapper.cpp` |
| Tray + per-pad status        | `src/ui/SystemTray.cpp`                    |
| OSD notification + help overlay | `src/ui/OsdOverlay.cpp`                  |
| XInput wrapper               | `src/win/XInputWrapper.cpp`                |
| SendInput/keybd_event helper | `src/win/SendInputHelper.cpp`              |

## Conventions Specific to This Codebase

- **Override triggers in both modes**, button mapping only in Mouse mode. That's the split inside `InputMapper`.
- **Default action enum strings** use the `actionToString` ↔ `stringToAction` pair in `core/Types.cpp`. The settings dialog uses the same enum; `actionToChinese()` powers the help overlay text.
- **Hot-reload from `Config` is the canonical way to apply changes** — the `SettingsDialog` doesn't poke subsystems directly, it just edits the JSON and lets the watcher fan out.
- **`m_ltTabBlocked` and friends** are button-level debounce flags inside `KeyboardMapper` for tricky presses like Alt+Tab (where Alt must remain held). Add new "sticky modifier" actions there.
- **Sustained CPU/GPU usage** in `GameDetector` uses accumulator windows (`m_cpuSustainedAccumMs`) — instant spikes won't trigger an auto-switch.
