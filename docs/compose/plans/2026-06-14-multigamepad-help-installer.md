# Multi-Gamepad + Help Screen + Installer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add multi-gamepad independent mode support, help screen hotkey (LT+R3), and Inno Setup installer.

**Architecture:** GamepadPoller polls 4 XInput controllers; each gets its own InputMapper + ComboKeyDetector + ModeManager instance in Application. LT+R3 triggers ShowHelp action displayed via enlarged OsdOverlay. Inno Setup script packages the build output.

**Tech Stack:** Qt 6.11.1, MinGW 13.1.0, XInput, SendInput, Inno Setup 6

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `src/core/Types.h` | Modify | Add `GamepadID` type alias, `ButtonAction::ShowHelp` |
| `src/gamepad/GamepadPoller.h` | Modify | Signal carries `controllerIndex` |
| `src/gamepad/GamepadPoller.cpp` | Modify | Loop 0-3, emit per-controller |
| `src/gamepad/ComboKeyDetector.h` | Modify | Accept `controllerIndex` param |
| `src/gamepad/ComboKeyDetector.cpp` | Modify | Accept `controllerIndex` param |
| `src/input/InputMapper.h` | Modify | Accept `controllerIndex` in signal |
| `src/input/InputMapper.cpp` | Modify | Accept `controllerIndex` in slot |
| `src/input/KeyboardMapper.h` | Modify | Accept `controllerIndex` for per-pad blocking state |
| `src/input/KeyboardMapper.cpp` | Modify | Per-pad LT/RT/tab-blocked state, ShowHelp action |
| `src/core/ModeManager.h` | Modify | Accept `controllerIndex` in signals |
| `src/core/ModeManager.cpp` | Modify | Emit per-pad mode changes |
| `src/ui/OsdOverlay.h` | Modify | Support multi-line help text, longer display |
| `src/ui/OsdOverlay.cpp` | Modify | Multi-line paint, configurable duration |
| `src/ui/SystemTray.h` | Modify | Show per-pad mode in menu |
| `src/ui/SystemTray.cpp` | Modify | Dynamic menu for pad status |
| `src/app/Application.h` | Modify | Arrays of InputMapper/ComboKeyDetector/ModeManager |
| `src/app/Application.cpp` | Modify | Wire 4x instances, help signal routing |
| `config/default_config.json` | Modify | Add `ShowHelp` to LT+R3 mapping |
| `installer/GamepadMouseSim.iss` | Create | Inno Setup script |

---

### Task 1: Add ShowHelp action and GamepadID type to Types.h

**Files:**
- Modify: `src/core/Types.h:57-58`
- Modify: `src/core/Types.h:100-102`

- [ ] **Step 1: Add ShowHelp to ButtonAction enum**

In `src/core/Types.h`, add after `ScrollRight` (line 57):

```cpp
    ScrollRight,
    ShowHelp
```

- [ ] **Step 2: Add string conversion for ShowHelp**

In `src/core/Types.cpp`, add case in `stringToAction`:

```cpp
    if (str == "ShowHelp") return ButtonAction::ShowHelp;
```

And in `actionToString`:

```cpp
    case ButtonAction::ShowHelp: return "ShowHelp";
```

- [ ] **Step 3: Commit**

```bash
git add src/core/Types.h src/core/Types.cpp
git commit -m "feat: add ShowHelp button action"
```

---

### Task 2: Modify GamepadPoller to poll all 4 controllers

**Files:**
- Modify: `src/gamepad/GamepadPoller.h:8-37`
- Modify: `src/gamepad/GamepadPoller.cpp:53-87`

- [ ] **Step 1: Update signal to include controller index**

In `src/gamepad/GamepadPoller.h`, change signal:

```cpp
signals:
    void gamepadStateChanged(int controllerIndex, const GamepadState& state);
    void gamepadConnected(int controllerIndex);
    void gamepadDisconnected(int controllerIndex);
```

- [ ] **Step 2: Add prev state array**

In `src/gamepad/GamepadPoller.h`, change member:

```cpp
private:
    void pollOnce();

    XInputWrapper m_xinput;
    PollThread m_thread;
    GamepadState m_prevState[4];
```

- [ ] **Step 3: Rewrite pollOnce to loop 4 controllers**

Replace entire `pollOnce` in `src/gamepad/GamepadPoller.cpp`:

```cpp
void GamepadPoller::pollOnce() {
    for (int i = 0; i < 4; ++i) {
        XINPUT_STATE xState;
        bool connected = m_xinput.getState(i, &xState);

        GamepadState state;
        state.connected = connected;

        if (connected) {
            float lx = xState.Gamepad.sThumbLX / 32767.0f;
            float ly = xState.Gamepad.sThumbLY / 32767.0f;
            float rx = xState.Gamepad.sThumbRX / 32767.0f;
            float ry = xState.Gamepad.sThumbRY / 32767.0f;

            float deadzone = 7849.0f / 32767.0f;
            applyDeadzone(lx, ly, deadzone, state.leftX, state.leftY);
            applyDeadzone(rx, ry, deadzone, state.rightX, state.rightY);

            state.leftTrigger = xState.Gamepad.bLeftTrigger / 255.0f;
            state.rightTrigger = xState.Gamepad.bRightTrigger / 255.0f;
            state.buttons = xState.Gamepad.wButtons;
        }

        uint16_t prevButtons = m_prevState[i].buttons;
        bool wasConnected = m_prevState[i].connected;
        state.prevButtons = prevButtons;
        m_prevState[i] = state;

        if (connected && !wasConnected) {
            emit gamepadConnected(i);
        } else if (!connected && wasConnected) {
            emit gamepadDisconnected(i);
        }

        emit gamepadStateChanged(i, state);
    }
}
```

- [ ] **Step 4: Commit**

```bash
git add src/gamepad/GamepadPoller.h src/gamepad/GamepadPoller.cpp
git commit -m "feat: poll all 4 XInput controllers independently"
```

---

### Task 3: Update InputMapper to accept controller index

**Files:**
- Modify: `src/input/InputMapper.h:10-27`
- Modify: `src/input/InputMapper.cpp:12-44`

- [ ] **Step 1: Update InputMapper constructor to take controllerIndex**

In `src/input/InputMapper.h`:

```cpp
class InputMapper : public QObject {
    Q_OBJECT
public:
    explicit InputMapper(Config* config, int controllerIndex, QObject* parent = nullptr);

public slots:
    void onGamepadStateChanged(int controllerIndex, const GamepadState& state);
    void onModeChanged(GamepadMode mode);
    void onConfigChanged();

private:
    Config* m_config;
    int m_controllerIndex;
    MouseMapper m_mouseMapper;
    KeyboardMapper m_keyboardMapper;
    GamepadMode m_mode = GamepadMode::Mouse;
    float m_prevLeftTrigger = 0.0f;
    float m_prevRightTrigger = 0.0f;
};
```

- [ ] **Step 2: Update InputMapper.cpp**

In `src/input/InputMapper.cpp`:

```cpp
InputMapper::InputMapper(Config* config, int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_controllerIndex(controllerIndex)
    , m_mouseMapper(config, this)
    , m_keyboardMapper(config, this)
{
}

void InputMapper::onGamepadStateChanged(int controllerIndex, const GamepadState& state) {
    if (controllerIndex != m_controllerIndex) return;
    if (!state.connected || m_mode != GamepadMode::Mouse) return;

    m_mouseMapper.processLeftStick(state.leftX, state.leftY);
    m_mouseMapper.processRightStick(state.rightX, state.rightY);

    m_keyboardMapper.processTrigger(state.leftTrigger, state.rightTrigger,
                                    m_prevLeftTrigger, m_prevRightTrigger);
    m_prevLeftTrigger = state.leftTrigger;
    m_prevRightTrigger = state.rightTrigger;

    uint16_t buttonsToCheck[] = {
        XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
        XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
        XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
        XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START
    };

    for (uint16_t btn : buttonsToCheck) {
        bool isPressed = (state.buttons & btn) != 0;
        bool wasPressed = (state.prevButtons & btn) != 0;
        if (isPressed || wasPressed) {
            m_keyboardMapper.processButton(btn, isPressed, state.prevButtons);
        }
    }
}

void InputMapper::onModeChanged(GamepadMode mode) {
    m_mode = mode;
    qDebug() << "InputMapper Pad" << m_controllerIndex << "mode:" << (mode == GamepadMode::Mouse ? "Mouse" : "Default");
}

void InputMapper::onConfigChanged() {
    qDebug() << "InputMapper Pad" << m_controllerIndex << "config changed, reloading";
    m_mouseMapper.onConfigChanged();
    m_keyboardMapper.onConfigChanged();
}
```

- [ ] **Step 3: Commit**

```bash
git add src/input/InputMapper.h src/input/InputMapper.cpp
git commit -m "feat: InputMapper accepts controllerIndex for per-pad isolation"
```

---

### Task 4: Update ComboKeyDetector to accept controller index

**Files:**
- Modify: `src/gamepad/ComboKeyDetector.h:8-24`
- Modify: `src/gamepad/ComboKeyDetector.cpp:8-28`

- [ ] **Step 1: Add controllerIndex to signal**

In `src/gamepad/ComboKeyDetector.h`:

```cpp
class ComboKeyDetector : public QObject {
    Q_OBJECT
public:
    explicit ComboKeyDetector(int controllerIndex, QObject* parent = nullptr);

    void setHoldDuration(int ms) { m_holdDurationMs = ms; }

public slots:
    void onGamepadState(int controllerIndex, const GamepadState& state);

signals:
    void comboTriggered(int controllerIndex);

private:
    int m_controllerIndex;
    QElapsedTimer m_holdTimer;
    bool m_holding = false;
    int m_holdDurationMs = kDefaultComboHoldMs;
};
```

- [ ] **Step 2: Update ComboKeyDetector.cpp**

```cpp
ComboKeyDetector::ComboKeyDetector(int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_controllerIndex(controllerIndex)
{
}

void ComboKeyDetector::onGamepadState(int controllerIndex, const GamepadState& state) {
    if (controllerIndex != m_controllerIndex) return;
    if (!state.connected) {
        m_holding = false;
        return;
    }

    bool viewPressed = (state.buttons & XINPUT_GAMEPAD_BACK) != 0;
    bool menuPressed = (state.buttons & XINPUT_GAMEPAD_START) != 0;

    if (viewPressed && menuPressed) {
        if (!m_holding) {
            m_holding = true;
            m_holdTimer.start();
        } else if (m_holdTimer.elapsed() >= m_holdDurationMs) {
            m_holding = false;
            emit comboTriggered(m_controllerIndex);
        }
    } else {
        m_holding = false;
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add src/gamepad/ComboKeyDetector.h src/gamepad/ComboKeyDetector.cpp
git commit -m "feat: ComboKeyDetector per-pad index filtering"
```

---

### Task 5: Update ModeManager to emit per-pad mode changes

**Files:**
- Modify: `src/core/ModeManager.h:9-43`
- Modify: `src/core/ModeManager.cpp:23-34`

- [ ] **Step 1: Add controllerIndex to signals**

In `src/core/ModeManager.h`:

```cpp
class ModeManager : public QObject {
    Q_OBJECT
public:
    explicit ModeManager(Config* config, int controllerIndex, QObject* parent = nullptr);

    void start();
    GamepadMode currentMode() const { return m_mode; }
    bool isLocked() const { return m_locked; }
    bool isPaused() const { return m_paused; }
    int controllerIndex() const { return m_controllerIndex; }

public slots:
    void manualSwitch();
    void toggleLock();
    void togglePause();
    void onConfigChanged();

signals:
    void modeChanged(int controllerIndex, GamepadMode mode);

private slots:
    void onPollTimer();

private:
    void setMode(GamepadMode mode);
    void loadConfig();

    Config* m_config;
    int m_controllerIndex;
    GamepadMode m_mode = GamepadMode::Mouse;
    bool m_locked = false;
    bool m_paused = false;
    QTimer m_pollTimer;
    QTimer m_lockoutTimer;
    int m_pollIntervalMs = kDefaultPollingIntervalMs;
    int m_lockoutMs = kDefaultManualLockoutMs;
};
```

- [ ] **Step 2: Update ModeManager.cpp constructor and signals**

```cpp
ModeManager::ModeManager(Config* config, int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_controllerIndex(controllerIndex)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &ModeManager::onPollTimer);
    connect(&m_lockoutTimer, &QTimer::timeout, this, [this]() {
        qDebug() << "Lockout expired, resuming auto detection for Pad" << m_controllerIndex;
        m_pollTimer.start(m_pollIntervalMs);
    });
}

void ModeManager::manualSwitch() {
    if (m_locked) {
        qDebug() << "Manual switch blocked for Pad" << m_controllerIndex << "- mode is locked";
        return;
    }
    GamepadMode newMode = (m_mode == GamepadMode::Default) ? GamepadMode::Mouse : GamepadMode::Default;
    qDebug() << "Pad" << m_controllerIndex << "manual switch from"
             << (m_mode == GamepadMode::Mouse ? "Mouse" : "Default")
             << "to" << (newMode == GamepadMode::Mouse ? "Mouse" : "Default");
    setMode(newMode);
    m_pollTimer.stop();
    m_lockoutTimer.start(m_lockoutMs);
}

void ModeManager::setMode(GamepadMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        qDebug() << "Pad" << m_controllerIndex << "mode changed to:" << (mode == GamepadMode::Mouse ? "Mouse" : "Default");
        emit modeChanged(m_controllerIndex, mode);
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add src/core/ModeManager.h src/core/ModeManager.cpp
git commit -m "feat: ModeManager emits per-pad modeChanged signals"
```

---

### Task 6: Update OsdOverlay for multi-line help display

**Files:**
- Modify: `src/ui/OsdOverlay.h:8-25`
- Modify: `src/ui/OsdOverlay.cpp`

- [ ] **Step 1: Update OsdOverlay header**

```cpp
#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include "core/Types.h"

class OsdOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float windowOpacity READ windowOpacity WRITE setWindowOpacity)
public:
    explicit OsdOverlay(QWidget* parent = nullptr);

public slots:
    void showModeChange(int controllerIndex, GamepadMode mode);
    void showHelp(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void showMessage(const QString& text, int durationMs = 2000, int width = 300, int height = 60);
    bool m_isHelpMode = false;
    QString m_text;
    QPropertyAnimation m_fadeAnimation;
    QTimer m_hideTimer;
};
```

- [ ] **Step 2: Update OsdOverlay.cpp**

```cpp
#include "OsdOverlay.h"
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

OsdOverlay::OsdOverlay(QWidget* parent)
    : QWidget(parent)
    , m_fadeAnimation(this, "windowOpacity")
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(300, 60);

    m_fadeAnimation.setDuration(300);
    m_fadeAnimation.setStartValue(0.0);
    m_fadeAnimation.setEndValue(1.0);

    connect(&m_hideTimer, &QTimer::timeout, this, [this]() {
        m_fadeAnimation.stop();
        m_fadeAnimation.setDirection(QPropertyAnimation::Backward);
        m_fadeAnimation.start();
    });

    connect(&m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_fadeAnimation.direction() == QPropertyAnimation::Backward) {
            hide();
            m_isHelpMode = false;
        }
    });
}

void OsdOverlay::showModeChange(int controllerIndex, GamepadMode mode) {
    QString text = QString("Pad%1: %2").arg(controllerIndex + 1)
                   .arg(mode == GamepadMode::Mouse ? "Mouse" : "Default");
    qDebug() << "OSD showing:" << text;
    showMessage(text);
}

void OsdOverlay::showHelp(const QString& text) {
    m_isHelpMode = true;
    int lineCount = text.count('\n') + 1;
    int lineHeight = 18;
    int padding = 40;
    int w = 420;
    int h = qMin(lineCount * lineHeight + padding, 500);
    showMessage(text, 5000, w, h);
}

void OsdOverlay::showMessage(const QString& text, int durationMs, int width, int height) {
    m_text = text;

    m_hideTimer.stop();
    m_fadeAnimation.stop();

    setFixedSize(width, height);

    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.right() - width - 20, screenGeometry.bottom() - height - 20);
    }

    setWindowOpacity(0.0);
    show();
    raise();
    update();

    m_fadeAnimation.setDirection(QPropertyAnimation::Forward);
    m_fadeAnimation.start();

    m_hideTimer.start(durationMs);
}

void OsdOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);

    painter.setPen(Qt::white);
    if (m_isHelpMode) {
        QFont font("Consolas", 10);
        painter.setFont(font);
        painter.drawText(rect().adjusted(15, 15, -15, -15), Qt::AlignLeft | Qt::AlignTop, m_text);
    } else {
        painter.setFont(QFont("Segoe UI", 14, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, m_text);
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add src/ui/OsdOverlay.h src/ui/OsdOverlay.cpp
git commit -m "feat: OsdOverlay supports multi-line help display"
```

---

### Task 7: Add ShowHelp to KeyboardMapper with per-pad state

**Files:**
- Modify: `src/input/KeyboardMapper.h:10-43`
- Modify: `src/input/KeyboardMapper.cpp:7-231`

- [ ] **Step 1: Add controllerIndex to KeyboardMapper and ShowHelp signal**

In `src/input/KeyboardMapper.h`:

```cpp
class KeyboardMapper : public QObject {
    Q_OBJECT
public:
    explicit KeyboardMapper(Config* config, int controllerIndex, QObject* parent = nullptr);

    void processButton(uint16_t button, bool pressed, uint16_t prevButtons);
    void processTrigger(float leftTrigger, float rightTrigger,
                        float prevLeftTrigger, float prevRightTrigger);

public slots:
    void onConfigChanged();

signals:
    void scrollRequested(float dx, float dy);
    void showHelpRequested();

private:
    void loadConfig();
    void executeAction(ButtonAction action);
    QString buildHelpText();

    ButtonAction lookupAction(const QString& btnName);

    Config* m_config;
    int m_controllerIndex;
    QMap<QString, ButtonAction> m_directMapping;
    QMap<QString, QMap<QString, ButtonAction>> m_modifierMapping;

    bool m_lbHeld = false;
    bool m_rbHeld = false;
    bool m_ltHeld = false;
    bool m_rtHeld = false;

    QSet<uint16_t> m_pressedButtons;
    QSet<uint16_t> m_ltTabBlocked;
    bool m_ltTabActive = false;
};
```

- [ ] **Step 2: Update KeyboardMapper.cpp constructor and add ShowHelp handling**

```cpp
KeyboardMapper::KeyboardMapper(Config* config, int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_controllerIndex(controllerIndex)
{
    loadConfig();
}
```

- [ ] **Step 3: Add ShowHelp case in processTrigger**

After the RT release block in `processTrigger`, add:

```cpp
    // ShowHelp: LT+R3
    bool r3Pressed = m_pressedButtons.contains(XINPUT_GAMEPAD_RIGHT_THUMB);
    // This is handled in processButton, not here
```

- [ ] **Step 4: Add ShowHelp case in processButton's executeAction path**

In `processButton`, the `executeAction(action)` call at line 102 will handle it. We need to add the case in `executeAction`.

- [ ] **Step 5: Add ShowHelp case in executeAction**

```cpp
    case ButtonAction::ShowHelp: {
        QString helpText = buildHelpText();
        emit showHelpRequested();
        break;
    }
```

- [ ] **Step 6: Add buildHelpText method**

```cpp
QString KeyboardMapper::buildHelpText() {
    QStringList lines;
    lines << "=== Gamepad Mouse Sim ===";
    lines << "";
    lines << "--- Direct Mapping ---";
    for (auto it = m_directMapping.begin(); it != m_directMapping.end(); ++it) {
        if (it.value() != ButtonAction::None) {
            lines << QString("  %1 = %2").arg(it.key()).arg(actionToString(it.value()));
        }
    }
    lines << "";
    lines << "--- LT Layer (Hold LT) ---";
    if (m_modifierMapping.contains("LT")) {
        const auto& ltMap = m_modifierMapping.value("LT");
        for (auto it = ltMap.begin(); it != ltMap.end(); ++it) {
            if (it.value() != ButtonAction::None) {
                lines << QString("  LT+%1 = %2").arg(it.key()).arg(actionToString(it.value()));
            }
        }
    }
    lines << "  LT+R3 = Show Help";
    lines << "";
    lines << "--- RT Layer (Hold RT) ---";
    if (m_modifierMapping.contains("RT")) {
        const auto& rtMap = m_modifierMapping.value("RT");
        for (auto it = rtMap.begin(); it != rtMap.end(); ++it) {
            if (it.value() != ButtonAction::None) {
                lines << QString("  RT+%1 = %2").arg(it.key()).arg(actionToString(it.value()));
            }
        }
    }
    lines << "";
    lines << "View+Menu = Switch Mode";
    return lines.join("\n");
}
```

- [ ] **Step 7: Commit**

```bash
git add src/input/KeyboardMapper.h src/input/KeyboardMapper.cpp
git commit -m "feat: KeyboardMapper adds ShowHelp action and help text builder"
```

---

### Task 8: Update Application to wire 4x pad instances

**Files:**
- Modify: `src/app/Application.h:14-31`
- Modify: `src/app/Application.cpp:6-74`

- [ ] **Step 1: Update Application.h**

```cpp
#pragma once

#include <QObject>
#include <QApplication>
#include <array>
#include "core/Config.h"
#include "core/ModeManager.h"
#include "core/ProcessDetector.h"
#include "gamepad/GamepadPoller.h"
#include "gamepad/ComboKeyDetector.h"
#include "input/InputMapper.h"
#include "ui/SystemTray.h"
#include "ui/OsdOverlay.h"

constexpr int kMaxGamepads = 4;

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* parent = nullptr);
    ~Application();

    bool initialize();

private:
    Config m_config;
    ProcessDetector m_processDetector;
    GamepadPoller m_gamepadPoller;
    SystemTray m_systemTray;
    OsdOverlay m_osdOverlay;

    std::array<ComboKeyDetector*, kMaxGamepads> m_comboDetectors;
    std::array<InputMapper*, kMaxGamepads> m_inputMappers;
    std::array<ModeManager*, kMaxGamepads> m_modeManagers;
};
```

- [ ] **Step 2: Update Application.cpp**

```cpp
#include "Application.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

Application::Application(QObject* parent)
    : QObject(parent)
    , m_config(this)
    , m_processDetector(this)
    , m_gamepadPoller(this)
    , m_systemTray(this)
    , m_osdOverlay(nullptr)
{
    for (int i = 0; i < kMaxGamepads; ++i) {
        m_comboDetectors[i] = new ComboKeyDetector(i, this);
        m_inputMappers[i] = new InputMapper(&m_config, i, this);
        m_modeManagers[i] = new ModeManager(&m_config, i, this);
    }
}

Application::~Application() {
    m_gamepadPoller.stop();
}

bool Application::initialize() {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.json";
    qDebug() << "Looking for config at:" << configPath;

    bool loaded = m_config.load(configPath);
    if (!loaded) {
        configPath = QCoreApplication::applicationDirPath() + "/config/default_config.json";
        qDebug() << "Fallback to:" << configPath;
        loaded = m_config.load(configPath);
    }

    if (!loaded) {
        qDebug() << "WARNING: No config file found, using defaults";
    } else {
        qDebug() << "Config loaded successfully";
    }

    for (int i = 0; i < kMaxGamepads; ++i) {
        m_inputMappers[i]->onConfigChanged();
    }

    // GamepadPoller -> ComboKeyDetectors
    connect(&m_gamepadPoller, &GamepadPoller::gamepadStateChanged,
            this, [this](int idx, const GamepadState& state) {
                if (idx < kMaxGamepads) {
                    m_comboDetectors[idx]->onGamepadState(idx, state);
                }
            });

    // GamepadPoller -> InputMappers
    connect(&m_gamepadPoller, &GamepadPoller::gamepadStateChanged,
            this, [this](int idx, const GamepadState& state) {
                if (idx < kMaxGamepads) {
                    m_inputMappers[idx]->onGamepadStateChanged(idx, state);
                }
            });

    // ComboKeyDetectors -> ModeManagers
    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_comboDetectors[i], &ComboKeyDetector::comboTriggered,
                this, [this, i]() {
                    m_modeManagers[i]->manualSwitch();
                });
    }

    // ModeManagers -> InputMappers + SystemTray + OSD
    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_modeManagers[i], &ModeManager::modeChanged,
                this, [this, i](int idx, GamepadMode mode) {
                    m_inputMappers[idx]->onModeChanged(mode);
                    m_systemTray.onModeChanged(idx, mode);
                    m_osdOverlay.showModeChange(idx, mode);
                });
    }

    // SystemTray actions (switch all pads)
    connect(&m_systemTray, &SystemTray::switchModeRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->manualSwitch();
                }
            });
    connect(&m_systemTray, &SystemTray::lockModeRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->toggleLock();
                }
            });
    connect(&m_systemTray, &SystemTray::pauseRequested,
            this, [this]() {
                for (int i = 0; i < kMaxGamepads; ++i) {
                    m_modeManagers[i]->togglePause();
                }
            });
    connect(&m_systemTray, &SystemTray::exitRequested,
            qApp, &QApplication::quit);

    // Config hot-reload
    connect(&m_config, &Config::configChanged, this, [this]() {
        for (int i = 0; i < kMaxGamepads; ++i) {
            m_modeManagers[i]->onConfigChanged();
            m_inputMappers[i]->onConfigChanged();
        }
    });

    // ShowHelp from any pad
    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_inputMappers[i], &InputMapper::showHelpRequested,
                this, [this, i]() {
                    // build help from first connected pad's mapper
                    // (all share the same config, so help text is identical)
                    // The signal is emitted from KeyboardMapper, we need to route it
                });
    }

    m_gamepadPoller.start();
    for (int i = 0; i < kMaxGamepads; ++i) {
        m_modeManagers[i]->start();
    }
    m_systemTray.show();

    return true;
}
```

- [ ] **Step 3: Add showHelpRequested signal to InputMapper**

In `src/input/InputMapper.h`, add signal:

```cpp
signals:
    void showHelpRequested();
```

In `src/input/InputMapper.cpp`, connect KeyboardMapper's signal:

```cpp
InputMapper::InputMapper(Config* config, int controllerIndex, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_controllerIndex(controllerIndex)
    , m_mouseMapper(config, this)
    , m_keyboardMapper(config, this)
{
    connect(&m_keyboardMapper, &KeyboardMapper::showHelpRequested,
            this, &InputMapper::showHelpRequested);
}
```

- [ ] **Step 4: Update SystemTray to accept per-pad mode changes**

In `src/ui/SystemTray.h`:

```cpp
public slots:
    void onModeChanged(int controllerIndex, GamepadMode mode);
```

In `src/ui/SystemTray.cpp`:

```cpp
void SystemTray::onModeChanged(int controllerIndex, GamepadMode mode) {
    m_padModes[controllerIndex] = mode;
    m_padConnected[controllerIndex] = true;

    // Update tooltip with all pads
    QStringList padInfo;
    for (int i = 0; i < kMaxGamepads; ++i) {
        if (m_padConnected[i]) {
            padInfo << QString("P%1:%2").arg(i + 1)
                       .arg(m_padModes[i] == GamepadMode::Mouse ? "M" : "D");
        }
    }
    m_trayIcon.setToolTip("GamepadMouseSim - " + padInfo.join(" "));

    // Update status menu
    m_statusAction->setText(padInfo.join("  "));

    // Update icon based on first connected pad's mode
    for (int i = 0; i < kMaxGamepads; ++i) {
        if (m_padConnected[i]) {
            updateIcon(m_padModes[i]);
            break;
        }
    }
}
```

Add members to SystemTray.h:

```cpp
private:
    std::array<GamepadMode, kMaxGamepads> m_padModes;
    std::array<bool, kMaxGamepads> m_padConnected = {};
```

- [ ] **Step 5: Connect ShowHelp signal to OSD in Application.cpp**

Add after the ModeManagers wiring:

```cpp
    // ShowHelp from any pad -> OSD
    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_inputMappers[i], &InputMapper::showHelpRequested,
                &m_osdOverlay, [this, i]() {
                    // Build help text from any mapper (all share config)
                    // We call buildHelpText via a temporary or store it
                    // Actually, we need to get the help text from the mapper
                });
    }
```

Better approach: add a `helpText()` method to InputMapper:

In `src/input/InputMapper.h`:
```cpp
    QString helpText() const { return m_keyboardMapper.buildHelpText(); }
```

Make `buildHelpText()` public in KeyboardMapper.h.

Then in Application.cpp:
```cpp
    for (int i = 0; i < kMaxGamepads; ++i) {
        connect(m_inputMappers[i], &InputMapper::showHelpRequested,
                this, [this, i]() {
                    m_osdOverlay.showHelp(m_inputMappers[i]->helpText());
                });
    }
```

- [ ] **Step 6: Commit**

```bash
git add src/app/Application.h src/app/Application.cpp src/input/InputMapper.h src/input/InputMapper.cpp src/ui/SystemTray.h src/ui/SystemTray.cpp src/input/KeyboardMapper.h
git commit -m "feat: Application wires 4x pad instances with independent mode and help"
```

---

### Task 9: Update config for LT+R3 = ShowHelp

**Files:**
- Modify: `config/default_config.json:52`

- [ ] **Step 1: Change LT+R3 from Ctrl+Z to ShowHelp**

In `config/default_config.json`, change the LT modifier mapping:

```json
        "R3": "ShowHelp",
```

(replacing `"R3": "Ctrl+Z"`)

- [ ] **Step 2: Commit**

```bash
git add config/default_config.json
git commit -m "feat: LT+R3 mapped to ShowHelp instead of Ctrl+Z"
```

---

### Task 10: Create Inno Setup installer script

**Files:**
- Create: `installer/GamepadMouseSim.iss`

- [ ] **Step 1: Write Inno Setup script**

```ini
[Setup]
AppId={{B1E2A3C4-D5E6-7890-ABCD-EF1234567890}
AppName=Gamepad Mouse Simulator
AppVersion=1.0.0
AppPublisher=Xiaomi
DefaultDirName={autopf}\GamepadMouseSim
DefaultGroupName=Gamepad Mouse Simulator
OutputDir=installer
OutputBaseFilename=GamepadMouseSim-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "build\GamepadMouseSim.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "config\default_config.json"; DestDir: "{app}\config"; Flags: ignoreversion

[Icons]
Name: "{group}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"
Name: "{group}\{cm:UninstallProgram,Gamepad Mouse Simulator}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\GamepadMouseSim.exe"; Description: "{cm:LaunchProgram,Gamepad Mouse Simulator}"; Flags: nowait postinstall
```

- [ ] **Step 2: Commit**

```bash
git add installer/GamepadMouseSim.iss
git commit -m "feat: add Inno Setup installer script"
```

---

### Task 11: Build and verify

- [ ] **Step 1: Build the project**

```bash
cd D:\project\sbgj\build
cmake --build . --config Release
```

- [ ] **Step 2: Verify compilation succeeds**

Expected: Build successful with no errors.

- [ ] **Step 3: Run the application**

```bash
cd D:\project\sbgj\build
.\GamepadMouseSim.exe
```

Test checklist:
- Connect 1 gamepad → verify it works
- Connect 2 gamepads → verify both work independently
- Press View+Menu on Pad1 → only Pad1 switches mode
- Press View+Menu on Pad2 → only Pad2 switches mode
- Hold LT + press R3 → help screen appears
- Help screen shows all mappings
- Help screen fades after 5 seconds

- [ ] **Step 4: Final commit if needed**

```bash
git add -A
git commit -m "chore: build verification and final tweaks"
```
