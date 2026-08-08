#include "test_gamepad_poller.h"
#include "core/Config.h"
#include "core/ModeManager.h"
#include "gamepad/ComboKeyDetector.h"
#include "core/Types.h"
#include <QSignalSpy>
#include <QCoreApplication>
#include <QTemporaryFile>

static XINPUT_STATE makeFrame(uint16_t buttons) {
    XINPUT_STATE s{};
    s.Gamepad.wButtons = buttons;
    return s;
}

void TestGamepadPoller::init() {
    setXInputForTesting(&m_fake);
}

void TestGamepadPoller::cleanup() {
    setXInputForTesting(nullptr);
}

void TestGamepadPoller::ltView_hold1s_triggersModeSwitch() {
    // Build a 65-frame sequence for controller 0:
    //   Frames 0-4: no buttons (idle)
    //   Frames 5-64: L3 + View held together for ~1s (60 frames * 16ms)
    // ComboKeyDetector checks L3 (0x0040) + View (0x0020) held >= 1000ms
    constexpr uint16_t L3_VIEW = 0x0040 | 0x0020;
    std::vector<XINPUT_STATE> seq(65);
    for (int i = 0; i < 5; ++i)
        seq[i] = makeFrame(0); // idle
    for (int i = 5; i < 65; ++i)
        seq[i] = makeFrame(L3_VIEW); // L3+View held
    m_fake.setSequence(0, seq);

    // ModeManager requires a Config instance.
    QTemporaryFile tmpCfg(this);
    tmpCfg.open();
    tmpCfg.write("{}");
    tmpCfg.close();
    Config cfg(this);
    cfg.load(tmpCfg.fileName());

    // Create per-pad subsystems (same pattern as Application::initialize).
    int idx = 0;
    auto* cd = new ComboKeyDetector(idx, this);
    auto* mm = new ModeManager(&cfg, idx, this);
    // Set a zero hold duration so the combo triggers on the SECOND frame
    // deterministically. With QElapsedTimer measuring real time, a 1ms hold
    // is flaky in a tight Release loop where elapsed() may stay < 1ms for
    // the whole 65-frame burst.
    cd->setHoldDuration(0); // 0 ms — triggers on the second L3+View frame

    // Wire: ComboKeyDetector::comboTriggered → ModeManager::manualSwitch
    connect(cd, &ComboKeyDetector::comboTriggered, this, [mm](int cIdx) {
        if (cIdx == 0)
            mm->manualSwitch();
    });

    QSignalSpy modeSpy(mm, &ModeManager::modeChanged);

    // Feed the XINPUT frame sequence through ComboKeyDetector.
    for (size_t i = 0; i < seq.size(); ++i) {
        GamepadState gs;
        gs.connected = true;
        gs.buttons = seq[i].Gamepad.wButtons;
        gs.leftTrigger = seq[i].Gamepad.bLeftTrigger / 255.0f;

        cd->onGamepadState(idx, gs);
        QCoreApplication::processEvents();
    }

    // After the sequence, the combo detector should have triggered.
    // L3+View held for 60 frames (~1s) exceeds kDefaultComboHoldMs (1000ms).
    QCOMPARE_GT(modeSpy.count(), 0);
}