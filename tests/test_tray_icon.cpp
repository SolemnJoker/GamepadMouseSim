#include "test_tray_icon.h"

void TestTrayIcon::construct_doesNotThrow() {
    m_tray.reset(new SystemTray(this));
    QVERIFY(m_tray != nullptr);
}

void TestTrayIcon::icon_isNotNull() {
    m_tray.reset(new SystemTray(this));
    m_tray->show();

    QIcon icon = m_tray->trayIcon();
    QVERIFY2(!icon.isNull(), "tray icon should not be null after show()");

    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    QVERIFY2(!pixmap.isNull(), "icon should render to a non-null QPixmap at 32x32");
}
