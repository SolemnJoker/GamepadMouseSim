#include "KeyboardController.h"
#include "core/Config.h"
#include <QDebug>

namespace {

// buttonBitToName 的反向表。
uint16_t buttonNameToBit(const QString& name) {
    if (name == "A")
        return XINPUT_GAMEPAD_A;
    if (name == "B")
        return XINPUT_GAMEPAD_B;
    if (name == "X")
        return XINPUT_GAMEPAD_X;
    if (name == "Y")
        return XINPUT_GAMEPAD_Y;
    if (name == "DpadUp")
        return XINPUT_GAMEPAD_DPAD_UP;
    if (name == "DpadDown")
        return XINPUT_GAMEPAD_DPAD_DOWN;
    if (name == "DpadLeft")
        return XINPUT_GAMEPAD_DPAD_LEFT;
    if (name == "DpadRight")
        return XINPUT_GAMEPAD_DPAD_RIGHT;
    if (name == "LB")
        return XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (name == "RB")
        return XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (name == "L3")
        return XINPUT_GAMEPAD_LEFT_THUMB;
    if (name == "R3")
        return XINPUT_GAMEPAD_RIGHT_THUMB;
    if (name == "View")
        return XINPUT_GAMEPAD_BACK;
    if (name == "Menu")
        return XINPUT_GAMEPAD_START;
    return 0;
}

constexpr float kRtLayerThreshold = 0.6f; // 与 KeyboardMapper 的 RT 层判定一致

} // namespace

KeyboardController::KeyboardController(Config* config, KeyInjector& injector,
                                       IKeyboardOverlay* overlay, QObject* parent)
    : QObject(parent), m_config(config), m_overlay(overlay), m_nav(injector, this) {
    m_modes.fill(GamepadMode::Default);
    m_prevButtons.fill(0);

    connect(&m_nav, &KeyboardNavController::closeRequested, this, [this]() { closeOverlay(); });

    onConfigChanged();
}

void KeyboardController::onConfigChanged() {
    m_bindingDirty = true;
    m_nav.setRepeatTimings(m_config->value("keyboard.nav_repeat_delay_ms", 400).toInt(),
                           m_config->value("keyboard.nav_repeat_interval_ms", 150).toInt());
}

void KeyboardController::openOverlay() {
    if (m_overlay->isKeyboardVisible()) {
        closeOverlay();
        return;
    }
    m_nav.resetState();
    m_overlay->showKeyboard();
    qDebug() << "Keyboard overlay opened";
}

void KeyboardController::closeOverlay() {
    if (!m_overlay->isKeyboardVisible())
        return;
    m_nav.setDirection(0, 0);
    m_nav.resetState();
    m_overlay->hideKeyboard();
    qDebug() << "Keyboard overlay closed";
}

bool KeyboardController::onGamepadState(int controllerIndex, const GamepadState& state) {
    if (controllerIndex < 0 || controllerIndex >= kMaxGamepads)
        return false;

    if (!state.connected) {
        m_prevButtons[controllerIndex] = 0;
        return false;
    }

    if (m_modes[controllerIndex] != GamepadMode::Mouse) {
        m_prevButtons[controllerIndex] = state.buttons;
        return false;
    }

    if (m_overlay->isKeyboardVisible()) {
        // 关闭检测:绑定(层 + 按钮)上升沿。打开期间的 InputMapper 已被
        // 旁路,关闭只能由这里负责(design.md D6)。
        if (matchesToggle(findShowKeyboardBinding(), state)) {
            closeOverlay();
        } else {
            handleNavInput(state);
        }
        m_prevButtons[controllerIndex] = state.buttons;
        return true; // 旁路 InputMapper
    }

    m_prevButtons[controllerIndex] = state.buttons;
    return false; // 未打开:InputMapper 常规路由(打开检测在那里)
}

void KeyboardController::onModeChanged(int controllerIndex, GamepadMode mode) {
    if (controllerIndex < 0 || controllerIndex >= kMaxGamepads)
        return;
    m_modes[controllerIndex] = mode;
    // 任意手柄切出 Mouse 模式即关闭键盘(Default 模式保持纯透传)。
    if (mode != GamepadMode::Mouse && m_overlay->isKeyboardVisible())
        closeOverlay();
}

KeyboardController::Binding KeyboardController::findShowKeyboardBinding() {
    if (!m_bindingDirty && m_binding.valid)
        return m_binding;

    Binding binding;
    const QJsonObject direct = m_config->value("mouse_mode.button_mapping").toJsonObject();
    for (auto it = direct.begin(); it != direct.end(); ++it) {
        if (stringToAction(it.value().toString()) == ButtonAction::ShowKeyboard) {
            binding = Binding{true, QString(), it.key()};
            break;
        }
    }

    if (!binding.valid) {
        const QJsonObject mods = m_config->value("mouse_mode.modifier_mapping").toJsonObject();
        // 运行时层名 "LT"(物理上按住 L3)优先;兼容 GUI 历史写入的 "L3"。
        for (const QString& layerName :
             {QStringLiteral("LT"), QStringLiteral("L3"), QStringLiteral("RT")}) {
            const QJsonObject layer = mods.value(layerName).toObject();
            for (auto it = layer.begin(); it != layer.end(); ++it) {
                if (stringToAction(it.value().toString()) == ButtonAction::ShowKeyboard) {
                    binding = Binding{true, layerName, it.key()};
                    break;
                }
            }
            if (binding.valid)
                break;
        }
    }

    // 兜底默认(design.md D8):LT 层 Menu,与 default_config.json 同源。
    if (!binding.valid)
        binding = Binding{true, QStringLiteral("LT"), QStringLiteral("Menu")};

    m_binding = binding;
    m_bindingDirty = false;
    return binding;
}

bool KeyboardController::matchesToggle(const Binding& binding, const GamepadState& state) const {
    if (!binding.valid)
        return false;
    const uint16_t bit = buttonNameToBit(binding.button);
    if (bit == 0)
        return false;

    const bool rising = (state.buttons & bit) != 0 && (state.prevButtons & bit) == 0;
    if (!rising)
        return false;

    if (binding.layer.isEmpty())
        return true; // 直接映射:按钮上升沿即触发
    if (binding.layer == QLatin1String("LT") || binding.layer == QLatin1String("L3"))
        return (state.buttons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    if (binding.layer == QLatin1String("RT"))
        return state.rightTrigger > kRtLayerThreshold;
    return false;
}

void KeyboardController::handleNavInput(const GamepadState& state) {
    // 十字键优先,其次摇杆量化(允许对角)。这些是"持续状态",
    // setDirection 内部处理"方向变化立即走一格、持续按住定时重复"。
    int dx = 0;
    int dy = 0;
    if (state.buttons & XINPUT_GAMEPAD_DPAD_LEFT)
        dx = -1;
    else if (state.buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
        dx = 1;
    if (state.buttons & XINPUT_GAMEPAD_DPAD_UP)
        dy = -1;
    else if (state.buttons & XINPUT_GAMEPAD_DPAD_DOWN)
        dy = 1;

    if (dx == 0 && dy == 0) {
        if (qAbs(state.leftX) > 0.5f)
            dx = state.leftX > 0 ? 1 : -1;
        if (qAbs(state.leftY) > 0.5f)
            // XInput 原始 Y 轴向上为正(MouseMapper 同样取反),屏幕坐标
            // 向下为正,所以这里与 MouseMapper 一致对 Y 取向。
            dy = state.leftY > 0 ? -1 : 1;
    }
    m_nav.setDirection(dx, dy);

    const bool aRising =
        (state.buttons & XINPUT_GAMEPAD_A) != 0 && (state.prevButtons & XINPUT_GAMEPAD_A) == 0;
    const bool bRising =
        (state.buttons & XINPUT_GAMEPAD_B) != 0 && (state.prevButtons & XINPUT_GAMEPAD_B) == 0;
    if (aRising)
        m_nav.confirmKey();
    if (bRising)
        m_nav.backspaceKey();
}
