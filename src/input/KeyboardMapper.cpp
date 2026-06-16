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

    if (m_l3Held && button == XINPUT_GAMEPAD_BACK) return;

    if (button == XINPUT_GAMEPAD_LEFT_THUMB) {
        if (pressed) {
            m_l3Held = true;
        } else {
            if (m_l3TabActive) {
                keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
            }
            m_l3Held = false;
            m_l3TabActive = false;
            m_l3TabBlocked.clear();
            m_lockedActions.clear();
        }
        return;
    }

    bool wasPressed = m_pressedButtons.contains(button);

    if (pressed) {
        m_pressedButtons.insert(button);
    } else {
        m_pressedButtons.remove(button);
        m_lockedActions.remove(button);
    }

    bool risingEdge = pressed && !wasPressed;

    ButtonAction action;
    if (!risingEdge && m_lockedActions.contains(button)) {
        action = m_lockedActions.value(button);
    } else {
        action = lookupAction(btnName);
        if (risingEdge && m_l3Held) {
            m_lockedActions[button] = action;
        }
    }

    qDebug() << "Btn:" << btnName << "pressed=" << pressed
             << "rising=" << risingEdge
             << "action=" << static_cast<int>(action);

    bool isL3Tab = m_l3Held && (action == ButtonAction::KeyTab || action == ButtonAction::KeyShiftTab);
    bool isL3AltTab = m_l3Held && action == ButtonAction::KeyAltTab;

    if (isL3Tab && m_l3TabBlocked.contains(button)) {
        if (!pressed) {
            m_l3TabBlocked.remove(button);
            qDebug() << "L3 Tab unblocked for" << btnName;
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
        if (isL3AltTab) {
            m_l3TabActive = true;
            keybd_event(VK_MENU, 0, 0, 0);
            keybd_event(VK_TAB, 0, 0, 0);
            keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
            qDebug() << "Sent Alt+Tab (L3 layer)";
            return;
        }
        if (isL3Tab) {
            m_l3TabBlocked.insert(button);
            qDebug() << "L3 Tab blocked for" << btnName;
            if (m_l3TabActive) {
                keybd_event(VK_TAB, 0x0F, 0, 0);
                keybd_event(VK_TAB, 0x0F, KEYEVENTF_KEYUP, 0);
                qDebug() << "Sent Tab for cycling";
            } else {
                m_l3TabActive = true;
                keybd_event(VK_MENU, 0, 0, 0);
                keybd_event(VK_TAB, 0, 0, 0);
                keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
                qDebug() << "Sent Alt+Tab (L3 layer)";
            }
            return;
        }
        executeAction(action);
    }
}

void KeyboardMapper::processTrigger(float leftTrigger, float rightTrigger,
                                     float prevLeftTrigger, float prevRightTrigger) {
    Q_UNUSED(leftTrigger);
    Q_UNUSED(prevLeftTrigger);

    if (rightTrigger > 0.6f && !m_rtHeld) {
        m_rtHeld = true;
        m_ctrlSent = false;
        qDebug() << "RT pressed";
    } else if (rightTrigger < 0.3f && m_rtHeld) {
        m_rtHeld = false;
        m_lockedActions.clear();
        if (m_ctrlSent) {
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = VK_CONTROL;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            m_ctrlSent = false;
            qDebug() << "RT released, Ctrl UP";
        } else {
            qDebug() << "RT released (no Ctrl sent)";
        }
    }
}

void KeyboardMapper::releaseModifiers() {
    if (m_l3Held) {
        if (m_l3TabActive) {
            keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        }
        m_l3Held = false;
        m_l3TabActive = false;
        m_l3TabBlocked.clear();
        m_lockedActions.clear();
        qDebug() << "Forced L3 release on mode change";
    }
    if (m_rtHeld) {
        m_rtHeld = false;
        m_lockedActions.clear();
        if (m_ctrlSent) {
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = VK_CONTROL;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            m_ctrlSent = false;
            qDebug() << "Forced Ctrl UP on mode change";
        }
    }
}

ButtonAction KeyboardMapper::lookupAction(const QString& btnName) {
    if (m_l3Held && m_modifierMapping.contains("L3")) {
        const auto& l3Map = m_modifierMapping.value("L3");
        if (l3Map.contains(btnName)) return l3Map.value(btnName);
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
    case ButtonAction::KeyCtrlW: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'W'); break;
    case ButtonAction::KeyCtrlA: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'A'); break;
    case ButtonAction::KeyCtrlC: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'C'); break;
    case ButtonAction::KeyCtrlV: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'V'); break;
    case ButtonAction::KeyCtrlX: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'X'); break;
    case ButtonAction::KeyCtrlZ: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'Z'); break;
    case ButtonAction::KeyCtrlS: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, 'S'); break;
    case ButtonAction::KeyCtrlShiftZ: {
        m_ctrlSent = true;
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
    case ButtonAction::KeyCtrlTab: m_ctrlSent = true; SendInputHelper::keyCombo(VK_CONTROL, VK_TAB); break;
    case ButtonAction::KeyCtrlShiftTab: {
        m_ctrlSent = true;
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
    case ButtonAction::ShowHelp: {
        emit showHelpRequested();
        break;
    }
    case ButtonAction::KeyWin: SendInputHelper::keyPress(VK_LWIN); SendInputHelper::keyRelease(VK_LWIN); break;
    default: break;
    }
}

static int visualWidth(const QString& s) {
    int w = 0;
    for (const QChar& c : s) {
        w += (c.unicode() > 0x7F) ? 2 : 1;
    }
    return w;
}

static QString padRight(const QString& s, int targetWidth) {
    int cur = visualWidth(s);
    return s + QString(qMax(0, targetWidth - cur), QChar(' '));
}

QStringList KeyboardMapper::buildHelpLines() const {
    QStringList lines;
    lines << "  手柄鼠标模拟器" << "";
    lines << "直接映射";
    for (auto it = m_directMapping.begin(); it != m_directMapping.end(); ++it) {
        if (it.value() != ButtonAction::None) {
            lines << "  " + padRight(it.key(), 10) + "→  " + actionToChinese(it.value());
        }
    }
    lines << "";
    lines << "L3层 (按住L3)";
    if (m_modifierMapping.contains("L3")) {
        const auto& l3Map = m_modifierMapping.value("L3");
        for (auto it = l3Map.begin(); it != l3Map.end(); ++it) {
            if (it.value() != ButtonAction::None) {
                lines << "  " + padRight("L3+" + it.key(), 14) + "→  " + actionToChinese(it.value());
            }
        }
    }
    lines << "  " + padRight("L3+View", 14) + "→  切换模式";
    lines << "  " + padRight("L3+R3", 14) + "→  显示帮助";
    lines << "";
    lines << "RT层 (按住RT)";
    if (m_modifierMapping.contains("RT")) {
        const auto& rtMap = m_modifierMapping.value("RT");
        for (auto it = rtMap.begin(); it != rtMap.end(); ++it) {
            if (it.value() != ButtonAction::None) {
                lines << "  " + padRight("RT+" + it.key(), 14) + "→  " + actionToChinese(it.value());
            }
        }
    }
    lines << "";
    lines << "摇杆";
    lines << "  " + padRight("左摇杆", 10) + "→  移动鼠标";
    lines << "  " + padRight("右摇杆", 10) + "→  滚动页面";
    lines << "";
    lines << "模式切换";
    lines << "  " + padRight("L3+View(长按1秒)", 20) + "→  切换模式";
    return lines;
}
