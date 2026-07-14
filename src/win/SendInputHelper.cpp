#include "SendInputHelper.h"

SendInputHelper::SendInputHelper(QObject* parent) : QObject(parent) {}

void SendInputHelper::moveMouse(int dx, int dy) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::leftClick() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void SendInputHelper::rightClick() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void SendInputHelper::middleClick() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void SendInputHelper::leftDown() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::leftUp() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::rightDown() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::rightUp() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::scrollVertical(int delta) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = delta * WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::scrollHorizontal(int delta) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
    input.mi.mouseData = delta * WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::keyPress(WORD vk) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::keyRelease(WORD vk) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void SendInputHelper::keyCombo(WORD mod, WORD key) {
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = mod;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = key;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = key;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = mod;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}

void SendInputHelper::mediaKey(WORD vk) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}
