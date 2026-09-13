#include "test_offscreen_render.h"
#include <QWidget>

void TestOffscreenRender::grab_basic() {
    QWidget w;
    w.resize(120, 90);
    w.show();
    const QPixmap pm = w.grab();
    QVERIFY(!pm.isNull());
    QCOMPARE(pm.size(), QSize(120, 90));
}

QTEST_MAIN(TestOffscreenRender)
#include "test_offscreen_render.moc"
