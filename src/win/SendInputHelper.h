#pragma once

#include <QObject>
#include <windows.h>

class SendInputHelper : public QObject {
    Q_OBJECT
  public:
    explicit SendInputHelper(QObject* parent = nullptr);

    static void moveMouse(int dx, int dy);
    static void leftClick();
    static void rightClick();
    static void middleClick();
    static void leftDown();
    static void leftUp();
    static void rightDown();
    static void rightUp();
    static void scrollVertical(int delta);
    static void scrollHorizontal(int delta);
    static void keyPress(WORD vk);
    static void keyRelease(WORD vk);
    static void keyCombo(WORD mod, WORD key);
    static void mediaKey(WORD vk);
};
