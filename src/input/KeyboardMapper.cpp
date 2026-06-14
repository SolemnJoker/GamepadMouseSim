#include "KeyboardMapper.h"
#include "core/Config.h"
#include "win/SendInputHelper.h"
#include <windows.h>
#include <QDebug>

KeyboardMapper::KeyboardMapper(Config* config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    loadConfig();
}

void KeyboardMapper::processButton(uint16_t button, bool pressed, uint16_t prevButtons) {
    QString btnName = buttonBitToName(button);
    if (btnName.isEmpty()) return;

    bool wasPressed = m_pressedButtons.contains(button);

    if (pressed) {
        m_pressedButtons.insert(button);
    } else {
        m_pressedButtons.remove(button);
    }

    bool risingEdge = pressed && !wasPressed;

    ButtonAction action = lookupAction(btnName);

    qDebug() << "Btn:" << btnName << "pressed=" << pressed
             << "rising=" << risingEdge
             << "action=" << static_cast<int>(action);

    bool isLtTab = m_ltHeld && (action == ButtonAction::KeyTab || action == ButtonAction::KeyShiftTab);

    if (isLtTab && m_ltTabBlocked.contains(button)) {
        if (!pressed) {
            m_ltTabBlocked.remove(button);
            qDebug() << "LT Tab unblocked for" << btnName;
        }
        return;
    }

    if (action == ButtonAction::MouseLeftHold) {
        if (pressed && !m_lbHeld) {
            SendInputHelper::leftDown();
            m_lbHeld = true;
        } else if (!pressed && m_lbHeld) {
            SendInputHelper::leftUp();
            m_lbHeld = false;
        }
        return;
    }
    if (action == ButtonAction::MouseRightHold) {
        if (pressed && !m_rbHeld) {
            SendInputHelper::rightDown();
            m_rbHeld = true;
        } else if (!pressed && m_rbHeld) {
            SendInputHelper::rightUp();
            m_rbHeld = false;
        }
        return;
    }
    if (action == ButtonAction::ScrollUp) {
        if (risingEdge) emit scrollRequested(0, 3);
        return;
    }
    if (action == ButtonAction::ScrollDown) {
        if (risingEdge) emit scrollRequested(0, -3);
        return;
    }

    if (risingEdge) {
        if (isLtTab) {
            m_ltTabBlocked.insert(button);
            qDebug() << "LT Tab blocked for" << btnName;
            if (m_ltTabActive) {
                keybd_event(VK_TAB, 0x0F, 0, 0);
                keybd_event(VK_TAB, 0x0F, KEYEVENTF_KEYUP, 0);
                qDebug() << "Sent Tab for cycling";
            } else {
                m_ltTabActive = true;
                INPUT inputs[4] = {};
                inputs[0].type = INPUT_KEYBOARD;
                inputs[0].ki.wVk = VK_MENU;
                if (action == ButtonAction::KeyShiftTab) {
                    inputs[1].type = INPUT_KEYBOARD;
                    inputs[1].ki.wVk = VK_SHIFT;
                }
                int tabIdx = (action == ButtonAction::KeyShiftTab) ? 2 : 1;
                inputs[tabIdx].type = INPUT_KEYBOARD;
                inputs[tabIdx].ki.wVk = VK_TAB;
                inputs[tabIdx + 1].type = INPUT_KEYBOARD;
                inputs[tabIdx + 1].ki.wVk = VK_TAB;
                inputs[tabIdx + 1].ki.dwFlags = KEYEVENTF_KEYUP;
                int count = (action == ButtonAction::KeyShiftTab) ? 4 : 3;
                SendInput(count, inputs, sizeof(INPUT));
                qDebug() << "Sent Alt+Tab";
            }
            return;
        }
        executeAction(action);
    }
}

void KeyboardMapper::processTrigger(float leftTrigger, float rightTrigger,
                                     float prevLeftTrigger, float prevRightTrigger) {
    if (leftTrigger > 0.5f && !m_ltHeld) {
        m_ltHeld = true;
        qDebug() << "LT pressed";
    } else if (leftTrigger <= 0.5f && m_ltHeld) {
        m_ltHeld = false;
        m_ltTabActive = false;
        m_ltTabBlocked.clear();
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_MENU;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
        qDebug() << "LT released, Alt UP";
    }

    if (rightTrigger > 0.5f && !m_rtHeld) {
        m_rtHeld = true;
        qDebug() << "RT pressed";
    } else if (rightTrigger <= 0.5f && m_rtHeld) {
        m_rtHeld = false;
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_CONTROL;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
        qDebug() << "RT released, Ctrl UP";
    }
}

ButtonAction KeyboardMapper::lookupAction(const QString& btnName) {
    if (m_ltHeld && m_modifierMapping.contains("LT")) {
        const auto& ltMap = m_modifierMapping.value("LT");
        if (ltMap.contains(btnName)) return ltMap.value(btnName);
    }
    if (m_rtHeld && m_modifierMapping.contains("RT")) {
        const auto& rtMap = m_modifierMapping.value("RT");
        if (rtMap.contains(btnName)) return rtMap.value(btnName);
    }
    return m_directMapping.value(btnName, ButtonAction::None);
}

void KeyboardMapper::onConfigChanged() { loadConfig(); }

void KeyboardMapper::loadConfig() {
    m_directMapping.clear();
    m_modifierMapping.clear();
    QJsonObject directObj = m_config->value("mouse_mode.button_mapping").toJsonObject();
    for (auto it = directObj.begin(); it != directObj.end(); ++it)
        m_directMapping[it.key()] = stringToAction(it.value().toString());
    qDebug() << "Loaded" << m_directMapping.size() << "direct mappings";

    QJsonObject modObj = m_config->value("mouse_mode.modifier_mapping").toJsonObject();
    for (auto modIt = modObj.begin(); modIt != modObj.end(); ++modIt) {
        QMap<QString, ButtonAction> layer;
        QJsonObject layerObj = modIt.value().toObject();
        for (auto btnIt = layerObj.begin(); btnIt != layerObj.end(); ++btnIt)
            layer[btnIt.key()] = stringToAction(btnIt.value().toString());
        m_modifierMapping[modIt.key()] = layer;
        qDebug() << "Loaded" << layer.size() << "mappings for modifier" << modIt.key();
    }
}

void KeyboardMapper::executeAction(ButtonAction action) {
    switch (action) {
    case ButtonAction::MouseLeftClick: SendInputHelper::leftClick(); break;
    case ButtonAction::MouseRightClick: SendInputHelper::rightClick(); break;
    case ButtonAction::MouseMiddleClick: SendInputHelper::middleClick(); break;
    case ButtonAction::KeyEnter: SendInputHelper::keyPress(VK_RETURN); SendInputHelper::keyRelease(VK_RETURN); break;
    case ButtonAction::KeyEscape: SendInputHelper::keyPress(VK_ESCAPE); SendInputHelper::keyRelease(VK_ESCAPE); break;
    case ButtonAction::KeyTab: SendInputHelper::keyPress(VK_TAB); SendInputHelper::keyRelease(VK_TAB); break;
    case ButtonAction::KeyBackspace: SendInputHelper::keyPress(VK_BACK); SendInputHelper::keyRelease(VK_BACK); break;
    case ButtonAction::KeyDelete: SendInputHelper::keyPress(VK_DELETE); SendInputHelper::keyRelease(VK_DELETE); break;
    case ButtonAction::KeyHome: SendInputHelper::keyPress(VK_HOME); SendInputHelper::keyRelease(VK_HOME); break;
    case ButtonAction::KeyEnd: SendInputHelper::keyPress(VK_END); SendInputHelper::keyRelease(VK_END); break;
    case ButtonAction::KeyPageUp: SendInputHelper::keyPress(VK_PRIOR); SendInputHelper::keyRelease(VK_PRIOR); break;
    case ButtonAction::KeyPageDown: SendInputHelper::keyPress(VK_NEXT); SendInputHelper::keyRelease(VK_NEXT); break;
    case ButtonAction::KeyArrowUp: SendInputHelper::keyPress(VK_UP); SendInputHelper::keyRelease(VK_UP); break;
    case ButtonAction::KeyArrowDown: SendInputHelper::keyPress(VK_DOWN); SendInputHelper::keyRelease(VK_DOWN); break;
    case ButtonAction::KeyArrowLeft: SendInputHelper::keyPress(VK_LEFT); SendInputHelper::keyRelease(VK_LEFT); break;
    case ButtonAction::KeyArrowRight: SendInputHelper::keyPress(VK_RIGHT); SendInputHelper::keyRelease(VK_RIGHT); break;
    case ButtonAction::KeyShiftTab: SendInputHelper::keyCombo(VK_SHIFT, VK_TAB); break;
    case ButtonAction::KeyAltTab: SendInputHelper::keyCombo(VK_MENU, VK_TAB); break;
    case ButtonAction::KeyAltF4: SendInputHelper::keyCombo(VK_MENU, VK_F4); break;
    case ButtonAction::KeyWinD: SendInputHelper::keyCombo(VK_LWIN, 'D'); break;
    case ButtonAction::KeyCtrlW: SendInputHelper::keyCombo(VK_CONTROL, 'W'); break;
    case ButtonAction::KeyCtrlA: SendInputHelper::keyCombo(VK_CONTROL, 'A'); break;
    case ButtonAction::KeyCtrlC: SendInputHelper::keyCombo(VK_CONTROL, 'C'); break;
    case ButtonAction::KeyCtrlV: SendInputHelper::keyCombo(VK_CONTROL, 'V'); break;
    case ButtonAction::KeyCtrlX: SendInputHelper::keyCombo(VK_CONTROL, 'X'); break;
    case ButtonAction::KeyCtrlZ: SendInputHelper::keyCombo(VK_CONTROL, 'Z'); break;
    case ButtonAction::KeyCtrlS: SendInputHelper::keyCombo(VK_CONTROL, 'S'); break;
    case ButtonAction::KeyCtrlShiftZ: {
        INPUT inputs[6] = {};
        inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = VK_CONTROL;
        inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = VK_SHIFT;
        inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = 'Z';
        inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = 'Z'; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[4].type = INPUT_KEYBOARD; inputs[4].ki.wVk = VK_SHIFT; inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[5].type = INPUT_KEYBOARD; inputs[5].ki.wVk = VK_CONTROL; inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(6, inputs, sizeof(INPUT));
        break;
    }
    case ButtonAction::KeyCtrlTab: SendInputHelper::keyCombo(VK_CONTROL, VK_TAB); break;
    case ButtonAction::KeyCtrlShiftTab: {
        INPUT inputs[6] = {};
        inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = VK_CONTROL;
        inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = VK_SHIFT;
        inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = VK_TAB;
        inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = VK_TAB; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[4].type = INPUT_KEYBOARD; inputs[4].ki.wVk = VK_SHIFT; inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[5].type = INPUT_KEYBOARD; inputs[5].ki.wVk = VK_CONTROL; inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(6, inputs, sizeof(INPUT));
        break;
    }
    case ButtonAction::KeyF5: SendInputHelper::keyPress(VK_F5); SendInputHelper::keyRelease(VK_F5); break;
    case ButtonAction::MediaPrevTrack: SendInputHelper::mediaKey(VK_MEDIA_PREV_TRACK); break;
    case ButtonAction::MediaNextTrack: SendInputHelper::mediaKey(VK_MEDIA_NEXT_TRACK); break;
    case ButtonAction::VolumeUp: SendInputHelper::mediaKey(VK_VOLUME_UP); break;
    case ButtonAction::VolumeDown: SendInputHelper::mediaKey(VK_VOLUME_DOWN); break;
    case ButtonAction::VolumeMute: SendInputHelper::mediaKey(VK_VOLUME_MUTE); break;
    case ButtonAction::KeyWin: SendInputHelper::keyPress(VK_LWIN); SendInputHelper::keyRelease(VK_LWIN); break;
    default: break;
    }
}
