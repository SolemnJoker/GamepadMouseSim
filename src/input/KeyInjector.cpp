#include "KeyInjector.h"
#include "win/SendInputHelper.h"

void SendKeyInjector::sendVk(WORD vk, bool withShift) {
    if (withShift) {
        SendInputHelper::keyCombo(VK_SHIFT, vk);
        return;
    }
    SendInputHelper::keyPress(vk);
    SendInputHelper::keyRelease(vk);
}

// B2:内嵌拼音引擎组词完成后经 KEYEVENTF_UNICODE 递交最终文本。
// B1 阶段无调用方——见 KeyInjector.h 顶部注释。
void SendKeyInjector::commitText(const QString& text) {
    Q_UNUSED(text);
}
