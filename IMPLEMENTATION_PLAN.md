# Gamepad Mouse Simulator - Implementation Plan

## 1. Project Structure

```
D:\project\sbgj\
├── CMakeLists.txt                  # Root CMake build file
├── README.md                       # User-facing readme
├── resources/
│   ├── icons/
│   │   ├── mouse_mode.ico          # System tray: mouse mode
│   │   ├── gamepad_mode.ico        # System tray: gamepad mode
│   │   ├── mouse_mode.png          # For OSD / .qrc
│   │   └── gamepad_mode.png
│   └── resources.qrc               # Qt resource file
├── config/
│   └── default_config.json         # Bundled default config
├── src/
│   ├── main.cpp                    # Entry point, QApplication setup
│   ├── app/
│   │   ├── Application.h           # Top-level singleton, owns everything
│   │   └── Application.cpp
│   ├── core/
│   │   ├── Config.h                # JSON config reader/writer + hot-reload
│   │   ├── Config.cpp
│   │   ├── ModeManager.h           # Mode state machine (default/mouse/locked/paused)
│   │   ├── ModeManager.cpp
│   │   ├── ProcessDetector.h       # Win32 process enumeration
│   │   ├── ProcessDetector.cpp
│   │   └── Types.h                 # Enums: GamepadMode, ButtonAction, etc.
│   ├── gamepad/
│   │   ├── GamepadPoller.h         # XInput polling thread (QThread)
│   │   ├── GamepadPoller.cpp
│   │   ├── ComboKeyDetector.h      # Detects View+Menu held combo
│   │   └── ComboKeyDetector.cpp
│   ├── input/
│   │   ├── InputMapper.h           # Maps gamepad state -> SendInput calls
│   │   ├── InputMapper.cpp
│   │   ├── MouseMapper.h           # Left stick -> mouse, right stick -> scroll
│   │   ├── MouseMapper.cpp
│   │   ├── KeyboardMapper.h        # Button -> keyboard/mouse action
│   │   └── KeyboardMapper.cpp
│   ├── ui/
│   │   ├── SystemTray.h            # QSystemTrayIcon + QMenu
│   │   ├── SystemTray.cpp
│   │   ├── OsdOverlay.h            # Semi-transparent fullscreen notification
│   │   ├── OsdOverlay.cpp
│   │   ├── SettingsDialog.h        # Optional settings GUI
│   │   └── SettingsDialog.cpp
│   └── win/
│       ├── XInputWrapper.h         # Thin wrapper around XInputGetState
│       ├── XInputWrapper.cpp
│       ├── SendInputHelper.h       # SendInput wrappers for mouse/keyboard
│       └── SendInputHelper.cpp
└── build/                          # Out-of-source build directory
```

## 2. Class Architecture

### 2.1 Core Classes

#### `Application` (app/Application.h)
- **Role**: Top-level orchestrator, owns all subsystems
- **Members**: Config*, ModeManager*, GamepadPoller*, InputMapper*, SystemTray*, OsdOverlay*
- **Startup sequence**: Load config → init ModeManager → start GamepadPoller → create SystemTray → connect signals
- **No main window** — app runs headless in system tray

#### `Config` (core/Config.h)
- **Role**: JSON config loading, saving, hot-reload via QFileSystemWatcher
- **Signals**: `configChanged()` emitted when file modified on disk
- **Key methods**: `load(path)`, `save()`, `value<T>(key, default)`, `set<T>(key, value)`
- Uses `QJsonDocument` for parsing
- Watches `config.json` with `QFileSystemWatcher`; debounces rapid saves (300ms timer)

#### `ModeManager` (core/ModeManager.h)
- **Role**: State machine for current operating mode
- **States**: `Default` (passthrough), `Mouse` (mapped), with flags `Locked`, `Paused`
- **Signals**: `modeChanged(GamepadMode)`, `autoDetectionResumed()`
- **Key methods**:
  - `setMode(mode)` — emits modeChanged
  - `lockCurrentMode()` / `unlockCurrentMode()` — disables auto-detection
  - `pauseAutoDetection(seconds)` — called after manual switch
  - `onProcessDetectionResult(bool targetFound)` — auto-switches based on result
- **Timer**: Internal `QTimer` for the lockout countdown (displays remaining in tray tooltip)

#### `ProcessDetector` (core/ProcessDetector.h)
- **Role**: Checks if any target process is running
- **Implementation**: Uses `CreateToolhelp32Snapshot` + `Process32First/Next`
- **Method**: `bool isTargetRunning(QStringList processes)` — returns true if ANY match found
- Called by `ModeManager`'s polling timer, NOT by a separate thread (the call is ~1ms)
- No need for a dedicated thread — `CreateToolhelp32Snapshot` is extremely fast

#### `Types` (core/Types.h)
```cpp
enum class GamepadMode { Default, Mouse };
enum class ButtonAction {
    None, MouseLeftClick, MouseRightClick, MouseMiddleClick,
    MouseLeftHold, MouseRightHold,
    KeyEnter, KeyEscape, KeyTab, KeyPageUp, KeyPageDown,
    KeyAltTab, KeyWinD, KeyCtrlW, KeyCtrlLeft, KeyCtrlRight,
    MediaPrevTrack, MediaNextTrack, KeyWin
};
struct GamepadState {
    bool connected;
    float leftX, leftY;     // -1.0 to 1.0
    float rightX, rightY;   // -1.0 to 1.0
    float leftTrigger;      // 0.0 to 1.0
    float rightTrigger;     // 0.0 to 1.0
    uint16_t buttons;       // XInput button bitmask
    uint16_t prevButtons;   // Previous frame for edge detection
};
```

### 2.2 Gamepad Classes

#### `GamepadPoller` (gamepad/GamepadPoller.h)
- **Role**: Dedicated QThread that polls XInput at ~60Hz and emits state updates
- **Why QThread**: Polling at 60Hz would block the main thread's event loop; offloading keeps UI responsive and meets the <1% CPU target
- **Implementation**:
  - Inherits `QThread`, runs a loop: `XInputGetState(0, &state)` every ~16ms
  - Computes delta from previous state, emits `gamepadStateChanged(GamepadState)` signal
  - Emits `gamepadConnected()` / `gamepadDisconnected()` on state transitions
  - Applies deadzone filtering inline before emitting
- **Cleanup**: `requestInterruption()` + `quit()` + `wait()` for clean shutdown

#### `ComboKeyDetector` (gamepad/ComboKeyDetector.h)
- **Role**: Detects when View+Menu are held simultaneously for 1 second
- **Input**: Receives `GamepadState` from `GamepadPoller`
- **Logic**: Tracks hold start time; if both buttons held >= threshold, emits `comboTriggered()`
- **State**: Resets if either button released before threshold
- Lives in main thread (receives signals), lightweight

### 2.3 Input Classes

#### `InputMapper` (input/InputMapper.h)
- **Role**: Central dispatcher — receives `GamepadState`, delegates to MouseMapper/KeyboardMapper based on current mode
- **Slots**: `onGamepadStateChanged(GamepadState)`, `onModeChanged(GamepadMode)`
- In `Default` mode: does nothing (pure passthrough — no XInput calls needed since Windows handles it)
- In `Mouse` mode: forwards to sub-mappers
- Manages mouse button hold state (LB/RB) — tracks press/release edges

#### `MouseMapper` (input/MouseMapper.h)
- **Role**: Left stick → cursor movement, Right stick → scroll wheel
- **Cursor movement**: Uses `SendInput` with `MOUSEEVENTF_MOVE`
  - Applies deadzone (configurable, default 15%)
  - Applies acceleration curve (quadratic by default: small deflection → slow, large → fast)
  - Sensitivity multiplier from config
  - Accumulates sub-pixel movement (float accumulator → integer SendInput)
- **Scroll**: Uses `SendInput` with `MOUSEEVENTF_WHEEL` / `MOUSEEVENTF_HWHEEL`
  - Accumulates float scroll amounts, sends when >= 1.0
  - Vertical from right stick Y, horizontal from right stick X
- **Button clicks**: Delegates to `SendInputHelper`

#### `KeyboardMapper` (input/KeyboardMapper.h)
- **Role**: Maps button presses to keyboard/mouse actions
- **Edge detection**: Only triggers on button press (rising edge), not hold
- **Exceptions**: LB/RB are hold-based (mouse drag), tracked as pressed state
- **Lookup**: Reads button mapping from Config, translates `ButtonAction` → `SendInput` sequences
- Uses `SendInputHelper` for all actual input injection

### 2.4 Windows API Classes

#### `XInputWrapper` (win/XInputWrapper.h)
- **Role**: Thin wrapper around `xinput1_4.dll` dynamic loading
- **Why dynamic loading**: Avoids hard dependency on XInput DLL; app still runs if no gamepad driver
- **Methods**: `isAvailable()`, `getState(DWORD userIndex, XINPUT_STATE* state)`
- Loads DLL with `LoadLibrary`/`GetProcAddress` on first use
- Links against `xinput1_4.lib` at build time (simpler; fallback to dynamic if needed)

#### `SendInputHelper` (win/SendInputHelper.h)
- **Role**: Stateless helper functions for `SendInput`
- **Mouse methods**: `moveMouse(dx, dy)`, `leftClick()`, `rightClick()`, `middleClick()`, `leftDown()`, `leftUp()`, `rightDown()`, `rightUp()`, `scrollVertical(delta)`, `scrollHorizontal(delta)`
- **Keyboard methods**: `keyPress(VK_CODE)`, `keyRelease(VK_CODE)`, `keyCombo(VK_CODE mod, VK_CODE key)`, `mediaKey(VK_CODE)`
- Each method constructs `INPUT` struct(s) and calls `SendInput`
- Handles modifier state (shift/ctrl/alt/win) for combos

### 2.5 UI Classes

#### `SystemTray` (ui/SystemTray.h)
- **Role**: System tray icon + right-click context menu
- **Icon states**: Mouse mode → mouse icon, Default mode → gamepad icon, Disconnected → gray icon
- **Menu items**:
  - "Mode: Mouse" / "Mode: Default" (gray, informational)
  - "Switch Mode" → triggers manual mode switch
  - "Lock Mode" (checkable) → locks current mode
  - "Pause Passthrough" (checkable) → temporarily pauses all mapping
  - separator
  - "Settings..." → opens SettingsDialog
  - "Exit" → quits application
- **Tooltip**: Shows current mode + controller connection status
- **Double-click**: Switches mode (convenience)

#### `OsdOverlay` (ui/OsdOverlay.h)
- **Role**: Full-screen semi-transparent overlay for mode change notification
- **Implementation**: `QWidget` with `Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool`
  - Background: semi-transparent black (`QColor(0, 0, 0, 160)`)
  - Centered text: "Mouse Mode" or "Default Mode"
  - Fades in/out with `QPropertyAnimation` on opacity
  - Auto-hides after configurable duration (default 2 seconds)
- **Position**: Bottom-right corner of primary screen
- Shows on mode change signal from ModeManager

#### `SettingsDialog` (ui/SettingsDialog.h)
- **Role**: Optional GUI for editing config (Phase 3+)
- **Layout**: Tab widget with tabs for General, Mouse Settings, Button Mapping
- **Early phases**: Just opens the JSON config file in notepad

### 2.6 Threading Model

```
Main Thread (Qt Event Loop)
├── Config (QFileSystemWatcher callbacks)
├── ModeManager (QTimer-driven polling)
├── ProcessDetector (called synchronously from ModeManager timer)
├── ComboKeyDetector (receives signals from GamepadPoller)
├── InputMapper (receives signals, calls SendInput — SendInput is thread-safe)
├── MouseMapper / KeyboardMapper
├── SystemTray
├── OsdOverlay
└── SettingsDialog

GamepadPoller Thread (QThread)
└── XInput polling loop (~60Hz)
    Emits signals cross-thread via Qt's queued connection
```

**Key decision**: All SendInput calls run on the **main thread**. `SendInput` is thread-safe and works from any thread, but keeping it on the main thread avoids complexity. The `GamepadPoller` only emits state; the main thread processes it and generates input.

**Why not separate input thread**: SendInput is fast (~1μs per call). The main thread's event loop has plenty of capacity. Adding another thread just for SendInput adds complexity with no measurable benefit.

## 3. Key Design Decisions

### 3.1 Polling vs Event-Driven for Gamepad

**Decision: Polling at ~60Hz on dedicated QThread**

Rationale:
- XInput is inherently polling-based (no event callback API exists)
- `XInputGetState` is cheap (~1μs per call)
- 60Hz polling is the standard for gamepad input (matches typical USB polling rate)
- QThread provides clean shutdown and signal/slot integration
- Alternative (raw input via `RegisterRawInputDevices`) only works for DInput, not XInput

### 3.2 Process Detection: Main Thread vs Worker Thread

**Decision: Main thread (synchronous call from QTimer)**

Rationale:
- `CreateToolhelp32Snapshot` + iteration over a short list (< 10 processes) takes < 1ms
- No need for async; the 2-second interval means this runs ~0.5 times/second average
- Avoids cross-thread coordination complexity

### 3.3 Config Hot-Reload

**Decision: QFileSystemWatcher on config.json**

- `QFileSystemWatcher::fileChanged` signal → debounced 300ms → re-parse JSON
- Config class emits `configChanged()` signal → all subsystems update live
- If file is deleted/renamed, re-add watch on next timer tick

### 3.4 Mouse Acceleration

**Decision: Quadratic curve by default, configurable**

- Raw stick value `v` ∈ [0, 1] after deadzone
- Accelerated value: `v² × sensitivity` (gives fine control at low deflection)
- Option to disable acceleration (linear: `v × sensitivity`)
- Accumulator pattern: float accumulated movement → integer pixel SendInput

### 3.5 Default Mode: True Passthrough

**Decision: In Default mode, do NOT call XInput at all**

Rationale:
- The requirement says "手柄所有输入完全原样输出，不做任何拦截、转换或映射"
- If we don't poll XInput, Windows handles the gamepad natively via XInput
- The game receives all input normally through the standard XInput pipeline
- This is the cleanest passthrough — no interference whatsoever
- **Exception**: We still need to detect the combo key (View+Menu) even in Default mode
  - Solution: Poll XInput only to detect the combo, but don't call SendInput or intercept anything else
  - Actually, in Default mode, the combo detection should still work since we're just reading state
  - Wait — if we don't poll XInput in Default mode, we can't detect the combo
  - **Revised**: Always poll XInput. In Default mode, read state only for combo detection. Never call SendInput.

### 3.6 Button Edge Detection

**Decision: Track previous frame's button state, detect rising edges**

- `GamepadState` carries both current and previous button bitmask
- `GamepadPoller` stores previous state, computes XOR for changed bits
- `KeyboardMapper` checks rising edge (was 0, now 1) for click/key actions
- Hold actions (LB/RB for drag) check current state directly

### 3.7 Deadzone Implementation

**Decision: Circular deadzone with smooth ramp**

- Compute stick magnitude: `sqrt(x² + y²)`
- If magnitude < deadzone threshold: output (0, 0)
- If magnitude >= deadzone: remap range [deadzone, 1.0] → [0, 1.0] with `output = (input - deadzone) / (1.0 - deadzone)`
- Preserve angle: scale both x and y by the same factor
- This prevents the "jump" at deadzone boundary

## 4. Build System (CMakeLists.txt)

```cmake
cmake_minimum_required(VERSION 3.20)
project(GamepadMouseSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Source files
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/core/Config.cpp
    src/core/ModeManager.cpp
    src/core/ProcessDetector.cpp
    src/gamepad/GamepadPoller.cpp
    src/gamepad/ComboKeyDetector.cpp
    src/input/InputMapper.cpp
    src/input/MouseMapper.cpp
    src/input/KeyboardMapper.cpp
    src/ui/SystemTray.cpp
    src/ui/OsdOverlay.cpp
    src/win/XInputWrapper.cpp
    src/win/SendInputHelper.cpp
)

set(HEADERS
    src/app/Application.h
    src/core/Config.h
    src/core/ModeManager.h
    src/core/ProcessDetector.h
    src/core/Types.h
    src/gamepad/GamepadPoller.h
    src/gamepad/ComboKeyDetector.h
    src/input/InputMapper.h
    src/input/MouseMapper.h
    src/input/KeyboardMapper.h
    src/ui/SystemTray.h
    src/ui/OsdOverlay.h
    src/win/XInputWrapper.h
    src/win/SendInputHelper.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    xinput1_4
)

# Windows-specific: hide console window in release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# Install rules (optional)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
```

## 5. Implementation Phases

### Phase 1: Foundation (Files: main.cpp, Application, Config, Types)
**Goal**: Build skeleton that compiles and runs with system tray

1. Set up CMakeLists.txt with Qt6 + MinGW
2. Create `Types.h` with enums and structs
3. Implement `Config` class (JSON read/write, hot-reload)
4. Implement minimal `Application` class (startup, shutdown)
5. Create `main.cpp` with QApplication
6. Verify build compiles with `cmake --build`

**Deliverable**: Empty app with system tray icon

### Phase 2: Gamepad Core (Files: GamepadPoller, XInputWrapper, ComboKeyDetector)
**Goal**: Read gamepad input and detect combo key

1. Implement `XInputWrapper` (dynamic XInput loading)
2. Implement `GamepadPoller` (QThread, 60Hz polling, deadzone)
3. Implement `ComboKeyDetector` (View+Menu held 1s)
4. Connect GamepadPoller signals to console output (debug)
5. Verify: running app shows gamepad state in console, combo detection works

**Deliverable**: App reads gamepad and detects combo key

### Phase 3: Input Mapping (Files: InputMapper, MouseMapper, KeyboardMapper, SendInputHelper)
**Goal**: Gamepad → mouse/keyboard works in mouse mode

1. Implement `SendInputHelper` (all mouse/keyboard primitives)
2. Implement `MouseMapper` (stick → cursor, stick → scroll, acceleration)
3. Implement `KeyboardMapper` (button → action lookup, edge detection)
4. Implement `InputMapper` (mode-based dispatch)
5. Verify: with mode forced to Mouse, gamepad controls mouse

**Deliverable**: Full mouse mode functionality

### Phase 4: Mode Management (Files: ModeManager, ProcessDetector)
**Goal**: Auto-detection and manual switching work

1. Implement `ProcessDetector` (Win32 process enumeration)
2. Implement `ModeManager` (state machine, auto-detection timer, manual switch, lockout)
3. Wire everything: Config → ModeManager → ProcessDetector → mode changes
4. Wire combo key → manual mode switch
5. Wire mode change → InputMapper enable/disable
6. Verify: add a test process name, see mode switch automatically

**Deliverable**: Auto and manual mode switching works

### Phase 5: System Tray & UI (Files: SystemTray, OsdOverlay)
**Goal**: Complete user-facing experience

1. Implement `SystemTray` (icon switching, context menu, actions)
2. Implement `OsdOverlay` (fade-in/out notification)
3. Wire tray menu actions → ModeManager
4. Wire mode changes → tray icon + OSD
5. Verify: full user flow works via tray

**Deliverable**: Complete application with tray + OSD

### Phase 6: Polish & Settings (Files: SettingsDialog, config enhancements)
**Goal**: User configuration and polish

1. Implement `SettingsDialog` (or notepad fallback initially)
2. Add autostart registry support
3. Add config validation and error handling
4. Performance testing (<1% CPU, <50MB RAM)
5. Edge cases: gamepad disconnect/reconnect, multiple controllers

**Deliverable**: Production-ready application

## 6. Signal/Slot Wiring Diagram

```
GamepadPoller::gamepadStateChanged(state) ──→ ComboKeyDetector::onGamepadState(state)
                                            ──→ InputMapper::onGamepadState(state)

ComboKeyDetector::comboTriggered() ──→ ModeManager::manualSwitch()

ProcessDetector::(called by ModeManager timer)
    → ModeManager::onProcessCheckResult(bool found)

ModeManager::modeChanged(mode) ──→ InputMapper::onModeChanged(mode)
                               ──→ SystemTray::onModeChanged(mode)
                               ──→ OsdOverlay::showModeChange(mode)

Config::configChanged() ──→ ModeManager::onConfigChanged()
                        ──→ InputMapper::onConfigChanged()
                        ──→ GamepadPoller::onConfigChanged()

SystemTray::switchModeRequested() ──→ ModeManager::manualSwitch()
SystemTray::lockModeRequested()   ──→ ModeManager::toggleLock()
SystemTray::pauseRequested()      ──→ ModeManager::togglePause()
SystemTray::exitRequested()       ──→ QApplication::quit()
```

## 7. Key Win32 API Usage

### XInput
```cpp
// In XInputWrapper.cpp
#include <xinput.h>
#pragma comment(lib, "xinput1_4.lib")

DWORD result = XInputGetState(0, &state);
if (result == ERROR_SUCCESS) {
    // Controller connected, state populated
}
```

### SendInput (Mouse)
```cpp
INPUT input = {};
input.type = INPUT_MOUSE;
input.mi.dx = dx;                    // PIXELS, not absolute
input.mi.dy = dy;
input.mi.mouseData = 0;
input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
// For absolute: dx/dy in 0-65535 range mapped to screen
// For relative: just MOUSEEVENTF_MOVE
SendInput(1, &input, sizeof(INPUT));
```

### SendInput (Keyboard)
```cpp
INPUT inputs[2] = {};
inputs[0].type = INPUT_KEYBOARD;
inputs[0].ki.wVk = VK_TAB;
inputs[0].ki.dwFlags = 0;           // Key down

inputs[1].type = INPUT_KEYBOARD;
inputs[1].ki.wVk = VK_TAB;
inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;  // Key up

SendInput(2, inputs, sizeof(INPUT));
```

### Process Enumeration
```cpp
HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
PROCESSENTRY32 pe = { sizeof(pe) };
if (Process32First(snapshot, &pe)) {
    do {
        if (_wcsicmp(pe.szExeFile, targetName) == 0) {
            // Found
        }
    } while (Process32Next(snapshot, &pe));
}
CloseHandle(snapshot);
```

## 8. File-by-File Implementation Notes

### `src/main.cpp`
- Create `QApplication` (no argc/argv needed for headless tray app)
- Set app name, org name for QSettings
- Create `Application` instance
- Connect `QApplication::lastWindowClosed` → quit (prevent premature exit)
- `app.exec()` then cleanup

### `src/core/Types.h`
- All enums, structs, constants
- XInput button bitmask constants: `XINPUT_GAMEPAD_A`, `XINPUT_GAMEPAD_B`, etc.
- Default config values as constexpr

### `src/core/Config.h/.cpp`
- Singleton pattern (or owned by Application)
- `load(const QString& path)` — reads JSON, populates internal QJsonObject
- `save()` — writes JSON back
- `value<T>(const QString& key, T default)` — dot-notation key lookup (e.g., "mouse_mode.left_stick.sensitivity_x")
- `set(const QString& key, const QVariant& value)` — set and auto-save
- Uses `QFileSystemWatcher` to monitor config file
- Debounce timer (300ms) to coalesce rapid saves

### `src/gamepad/GamepadPoller.h/.cpp`
- Constructor takes parent QObject
- `start()` — creates thread, starts polling
- `stop()` — requests interruption, quits, waits
- Polling loop:
  ```
  while (!isInterruptionRequested()) {
      XINPUT_STATE state;
      DWORD result = XInputGetState(0, &state);
      bool connected = (result == ERROR_SUCCESS);
      // Apply deadzone to sticks
      // Compute delta from previous state
      // Emit gamepadStateChanged(GamepadState)
      thread()->msleep(16);  // ~60Hz
  }
  ```
- Deadzone applied here (circular deadzone)

### `src/input/MouseMapper.h/.cpp`
- `processStick(float x, float y)` — applies sensitivity, acceleration, deadzone
- Accumulates sub-pixel movement (float xAccum, yAccum)
- `processScroll(float x, float y)` — accumulates scroll wheel ticks
- All output via SendInputHelper

### `src/ui/OsdOverlay.h/.cpp`
- Frameless, transparent, always-on-top widget
- `showMessage(const QString& text)` — positions at bottom-right, fades in, auto-hides
- Uses `QPropertyAnimation` on `windowOpacity`
- Background painted in `paintEvent` with rounded rect

## 9. Build & Run Commands

```bash
# Configure
cd D:\project\sbgj\build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . -j$(nproc)

# Run
./GamepadMouseSim.exe
```

## 10. Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| XInput polling CPU usage | 60Hz with sleep(16) is ~0.1% CPU; reduce to 30Hz if needed |
| SendInput blocked by UIPI | App runs at same integrity level as target; no admin needed |
| Config hot-reload race | Debounce timer + atomic read/write of QJsonObject |
| Gamepad disconnect during use | GamepadPoller detects and emits disconnected signal; InputMapper stops |
| Multiple XInput controllers | Only poll index 0 (Player 1) per requirements |
| Steam Big Picture detection | Parse process command line via `NtQueryInformationProcess` or just check for steam.exe (sufficient for MVP) |
| Overlay over fullscreen games | Qt `Qt::Tool` windows don't steal focus; may not show over exclusive fullscreen. Acceptable limitation. |
