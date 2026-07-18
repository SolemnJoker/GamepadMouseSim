# Testing & CI Standard — Design Spec

**Date:** 2026-07-18
**Status:** Approved for implementation
**Owner:** GamepadMouseSim maintainers
**Sub-project:** 5 of 5 under the project-standards initiative

---

## 1. Scope

### 1.1 In scope

- `IXInput` interface (analogous to `IRegistry`) for injecting fake gamepad state
- Global singleton accessor pattern (`setXInputForTesting` / `xInput()`)
- `FakeXInput` with frame-sequence timing simulation (simulates 60 Hz input sequences)
- Wiring test: LT+View hold → mode switch verification
- Test coverage rules and carve-out management strategy

### 1.2 Out of scope

| Concern | Reason |
|---|---|
| GitHub Actions CI workflow | User did not select |
| Build release artifacts CI | User did not select |
| Full UI testing framework (Squish, pytest-qt) | Unclear whether a GUI framework is worth the dependency; deferred until the IXInput-based tests prove the pattern |

---

## 2. IXInput Interface

### 2.1 Declaration

In `src/win/XInputWrapper.h`:

```cpp
class IXInput {
public:
    virtual ~IXInput() = default;
    virtual DWORD getState(DWORD dwUserIndex, XINPUT_STATE* pState) = 0;
};

IXInput* xInput();
void setXInputForTesting(IXInput* fake);
```

### 2.2 Production implementation

In `src/win/XInputWrapper.cpp`:

```cpp
namespace {
IXInput* g_testXInput = nullptr;
}

class RealXInput : public IXInput {
    DWORD getState(DWORD dwUserIndex, XINPUT_STATE* pState) override {
        return XInputGetState(dwUserIndex, pState);
    }
};

IXInput* xInput() {
    if (g_testXInput)
        return g_testXInput;
    static RealXInput real;
    return &real;
}

void setXInputForTesting(IXInput* fake) {
    g_testXInput = fake;
}
```

### 2.3 GamepadPoller integration

`src/gamepad/GamepadPoller.cpp` — replace `XInputGetState(i, &xState)` with `xInput()->getState(i, &xState)`.

This is a one-line change per call site. No other behavior changes.

---

## 3. FakeXInput with Frame-Sequence Simulation

### 3.1 Class design

In `tests/test_gamepad_poller.h`:

```cpp
#pragma once

#include "win/XInputWrapper.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

class FakeXInput : public IXInput {
public:
    // Set a per-controller sequence of XINPUT_STATE frames.
    // getState() advances through the sequence one frame per call.
    void setSequence(DWORD idx, const std::vector<XINPUT_STATE>& frames) {
        m_sequences[idx] = frames;
        m_cursors[idx] = 0;
    }

    DWORD getState(DWORD dwUserIndex, XINPUT_STATE* pState) override {
        auto it = m_sequences.find(dwUserIndex);
        if (it == m_sequences.end())
            return ERROR_DEVICE_NOT_CONNECTED;
        auto& frames = it->second;
        auto& cursor = m_cursors[dwUserIndex];
        if (cursor >= frames.size())
            cursor = frames.size() - 1; // hold last frame
        *pState = frames[cursor++];
        return ERROR_SUCCESS;
    }

private:
    std::unordered_map<DWORD, std::vector<XINPUT_STATE>> m_sequences;
    std::unordered_map<DWORD, size_t> m_cursors;
};
```

### 3.2 Test: LT+View mode switch

In `tests/test_gamepad_poller.cpp`:

Build a 60-frame sequence:
- Frame 0–59: LT held (bLeftTrigger=255)
- Frame 60: LT + View pressed (bLeftTrigger=255 + wButtons|=XINPUT_GAMEPAD_BACK)
- Frame 61+: hold

Drive `GamepadPoller::pollOnce()` at ~16 ms intervals.

Assert `ModeManager::currentMode()` changes from `Mouse` to `Default` (or vice versa).

The test requires wiring `ComboKeyDetector` + `ModeManager` locally (not through `Application::initialize()`).

### 3.3 Test wiring

```cpp
class TestGamepadPoller : public QObject {
    Q_OBJECT
private slots:
    void init() {
        setXInputForTesting(&m_fake);
        // Create per-pad subsystems (same pattern as Application::initialize)
        m_detector = new ComboKeyDetector(0, this);
        m_modeMgr = new ModeManager(nullptr, 0, this);
    }
    void cleanup() {
        setXInputForTesting(nullptr);
        m_detector = nullptr;
        m_modeMgr = nullptr;
    }
    void ltView_hold1s_triggersModeSwitch();

private:
    FakeXInput m_fake;
    ComboKeyDetector* m_detector = nullptr;
    ModeManager* m_modeMgr = nullptr;
};
```

---

## 4. Test Coverage Rules

### 4.1 Carve-out closure plan

| Spec §7.4 row | Carve-out | Closure by this sub-project? |
|---|---|---|
| 2 | LT+View → mode flip | YES — IXInput + timing simulation |
| 3 | LT+R3 → help overlay | Partially — need IInputMapper shim; deferred |
| 4 | Settings dialog UI | Needs UI test framework evaluation; deferred |

### 4.2 Coverage baseline

- All 5 existing ctest executables continue to pass
- New ctest: `test_gamepad_poller` (at least 1 case: mode switch via simulated hold)
- Every new feature PR must include a ctest case OR a documented carve-out in the spec

### 4.3 Test organization

| Directory | Contents |
|---|---|
| `tests/` (root) | All ctest executables. One `.cpp`/`.h` pair per test class. |
| No subdirectories | Keep flat until >15 test files |

### 4.4 Fake/interface convention

All test-injected interfaces follow the pattern established by `IRegistry` and `IXInput`:
- Interface declared alongside the production class header
- Global `set{Name}ForTesting()` accessor in the `.cpp` file
- `Fake{Name}` class in `tests/`
- `init()`/`cleanup()` pairs in the test class to install/restore the fake

---

## 5. Verification

- `ctest --test-dir build` — 6 test executables pass (was 5)
- New test `test_gamepad_poller` runs the LT+View hold sequence

---

## Appendix A — Cross-references

- **Code-architecture spec** §5 (code review): the test wiring pattern mirrors the Application::initialize() connection style.
- **Config spec** (sub-project 4): the version-mismatch test is an example of the timing-insensitive test pattern; the gamepad poller test extends this to timed sequences.
- **AGENTS.md Rule 2**: this sub-project closes the largest remaining carve-out.