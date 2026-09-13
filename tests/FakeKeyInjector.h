#pragma once

#include "input/KeyInjector.h"
#include <QString>
#include <vector>

// 记录型 Fake:替代 SendKeyInjector,使导航/控制器逻辑可在不触达
// Win32 输入队列的情况下断言注入序列(design.md D9)。
class FakeKeyInjector : public KeyInjector {
  public:
    struct Recorded {
        WORD vk;
        bool withShift;
    };

    void sendVk(WORD vk, bool withShift) override { keys.push_back({vk, withShift}); }
    void commitText(const QString& text) override { commits.push_back(text); }

    void clear() {
        keys.clear();
        commits.clear();
    }

    std::vector<Recorded> keys;
    std::vector<QString> commits;
};
