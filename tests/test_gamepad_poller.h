#pragma once

#include "win/XInputWrapper.h"
#include <QObject>
#include <QSignalSpy>
#include <unordered_map>
#include <vector>
#include <cstdint>

// In-memory fake that returns scripted XINPUT_STATE sequences per controller.
class FakeXInput : public IXInput {
  public:
    void setSequence(DWORD idx, const std::vector<XINPUT_STATE>& frames) {
        m_sequences[idx] = frames;
        m_cursors[idx] = 0;
    }

    bool getState(DWORD dwUserIndex, XINPUT_STATE* pState) override {
        auto it = m_sequences.find(dwUserIndex);
        if (it == m_sequences.end())
            return false;
        auto& frames = it->second;
        auto& cursor = m_cursors[dwUserIndex];
        if (cursor >= frames.size())
            cursor = frames.size() - 1;
        *pState = frames[cursor++];
        return true;
    }

  private:
    std::unordered_map<DWORD, std::vector<XINPUT_STATE>> m_sequences;
    std::unordered_map<DWORD, size_t> m_cursors;
};

class TestGamepadPoller : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void ltView_hold1s_triggersModeSwitch();

  private:
    FakeXInput m_fake;
};