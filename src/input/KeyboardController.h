#pragma once

#include "KeyInjector.h"
#include "KeyboardNavController.h"
#include "core/Types.h"
#include <QObject>
#include <array>

class Config;

// 键盘 overlay 的视图最小接口。生产实现是 ui/KeyboardOverlay(QWidget);
// 单测注入 FakeOverlay,保证控制器逻辑可在不创建真实窗口的情况下测试
// (design.md D9 逻辑/窗口分离)。
class IKeyboardOverlay {
  public:
    virtual ~IKeyboardOverlay() = default;
    virtual void showKeyboard() = 0;
    virtual void hideKeyboard() = 0;
    virtual bool isKeyboardVisible() const = 0;
};

// 虚拟键盘总控(design.md D6):
// - 开:InputMapper 在 Mouse 模式下识别 ShowKeyboard 动作 → emit
//   showKeyboardRequested → openOverlay()(复用 InputMapper 既有层逻辑,
//   避免在旁路层重复实现一层修饰键状态机);
// - 关:overlay 打开期间 InputMapper 被旁路,由本类自行识别"当前绑定"
//   的上升沿来关闭(绑定从配置反查,支持用户重映射);
// - 旁路:overlay 打开期间,原始 GamepadState 交 KeyboardNavController,
//   不再进入 InputMapper;模式切换组合键由独立的 ComboKeyDetector 承担,
//   不受影响(见 Application 的接线顺序)。
class KeyboardController : public QObject {
    Q_OBJECT
  public:
    KeyboardController(Config* config, KeyInjector& injector, IKeyboardOverlay* overlay,
                       QObject* parent = nullptr);

    // 返回 true 表示本帧输入已被键盘消费,调用方( Application )应跳过
    // InputMapper;false 表示常规路由照旧。
    bool onGamepadState(int controllerIndex, const GamepadState& state);
    void onModeChanged(int controllerIndex, GamepadMode mode);
    void onConfigChanged();

    // InputMapper::showKeyboardRequested → 这里。已打开时等价关闭(toggle)。
    void openOverlay();

    KeyboardNavController& nav() { return m_nav; }

  private:
    struct Binding {
        bool valid = false;
        QString layer; // "" = 直接映射;“LT”/“L3” = L3 按住层;“RT” = RT 扳机层
        QString button;
    };

    Binding findShowKeyboardBinding();
    bool matchesToggle(const Binding& binding, const GamepadState& state) const;
    void closeOverlay();
    void handleNavInput(const GamepadState& state);

    Config* m_config;
    IKeyboardOverlay* m_overlay;
    KeyboardNavController m_nav;
    std::array<GamepadMode, kMaxGamepads> m_modes{};
    std::array<uint16_t, kMaxGamepads> m_prevButtons{};
    Binding m_binding;
    bool m_bindingDirty = true;
};
