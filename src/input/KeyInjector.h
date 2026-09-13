#pragma once

#include <QString>
#include <windows.h>

// 文字递交层接口(design.md D3/D4)。
//
// B1 期只用 sendVk():发送"物理键盘等价"的 VK 事件,目标窗口当前的
// 输入法(微软拼音等)按物理键盘规则拦截组词——虚拟键盘对输入法透明。
//
// commitText() 是 B2 预留通道:KEYEVENTF_UNICODE(VK_PACKET)注入会把
// 字符直接落进目标窗口、绕过输入法组词。B1 严禁用它发字母/数字,否则
// 系统 IME 永远拿不到组词机会;它只服务于 B2 内嵌拼音引擎组词完成后的
// 最终上屏。
class KeyInjector {
  public:
    virtual ~KeyInjector() = default;

    // 发送单个虚拟键;withShift=true 时以 Shift+键 组合发送(组合不会
    // 触发输入法的"单击 Shift 切中英"歧义,见 design.md D7)。
    virtual void sendVk(WORD vk, bool withShift) = 0;

    // B2 专用:绕过输入法直接递交最终文本(UNICODE 通道)。
    virtual void commitText(const QString& text) = 0;
};

// 生产实现:经 SendInputHelper 发送。commitText 为 B2 预留空实现。
class SendKeyInjector : public KeyInjector {
  public:
    void sendVk(WORD vk, bool withShift) override;
    void commitText(const QString& text) override;
};
