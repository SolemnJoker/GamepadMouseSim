#include "app/Application.h"
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <windows.h>

static QFile* s_logFile = nullptr;

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = msg;
        break;
    case QtWarningMsg:
        txt = "WARN: " + msg;
        break;
    case QtCriticalMsg:
        txt = "CRIT: " + msg;
        break;
    case QtFatalMsg:
        txt = "FATAL: " + msg;
        break;
    default:
        txt = msg;
    }

    OutputDebugStringW((LPCWSTR)(txt + "\n").utf16());

    if (s_logFile) {
        QTextStream ts(s_logFile);
        ts << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " " << txt << "\n";
        s_logFile->flush();
    }
}

int main(int argc, char* argv[]) {
    // 进程级自检(design.md D7 L4)在 GUI 栈之前运行:配置链自检不需要
    // QApplication(offscreen 下 GUI 插件初始化曾致挂起),以退出码结束。
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--selftest") == 0) {
            extern bool runConfigSelftest();
            return runConfigSelftest() ? 0 : 1;
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName("GamepadMouseSim");
    app.setOrganizationName("GamepadMouseSim");
    app.setQuitOnLastWindowClosed(false);

    QString logPath = QApplication::applicationDirPath() + "/debug.log";
    s_logFile = new QFile(logPath);
    s_logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    qInstallMessageHandler(customMessageHandler);

    qDebug() << "=== Application starting ===";
    qDebug() << "App dir:" << QApplication::applicationDirPath();

    Application appLogic;
    if (!appLogic.initialize()) {
        qCritical() << "Failed to initialize";
        return 1;
    }

    qDebug() << "Entering event loop";
    int ret = app.exec();

    delete s_logFile;
    return ret;
}
