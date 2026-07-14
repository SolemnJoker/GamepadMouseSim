#include "test_log_severity.h"
#include <QDebug>
#include <QRegularExpression>

TestLogSeverity* TestLogSeverity::s_instance = nullptr;

void TestLogSeverity::messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    Q_UNUSED(ctx);
    if (s_instance) {
        s_instance->m_captured += QString::number(type) + ":" + msg + "\n";
    }
}

void TestLogSeverity::init() {
    s_instance = this;
    m_captured.clear();
    m_oldHandler = qInstallMessageHandler(messageHandler);
}

void TestLogSeverity::cleanup() {
    qInstallMessageHandler(m_oldHandler);
    s_instance = nullptr;
}

void TestLogSeverity::qWarning_capturesMessage() {
    qWarning() << "No config file found, using defaults";
    QVERIFY2(m_captured.contains("No config file found"),
             qPrintable(QString("Expected 'No config file found' in capture, got: %1").arg(m_captured)));
}

void TestLogSeverity::qCritical_capturesMessage() {
    qCritical() << "Failed to initialize";
    QVERIFY2(m_captured.contains("Failed to initialize"),
             qPrintable(QString("Expected 'Failed to initialize' in capture, got: %1").arg(m_captured)));
}

void TestLogSeverity::severityLevels_correctlyLabelled() {
    qWarning() << "warn1";
    qCritical() << "crit1";
    qDebug() << "debug1";

    // Verify that qWarning produces type 2 and qCritical produces type 3
    // (QtMsgType enum: QtDebugMsg=0, QtWarningMsg=1, QtCriticalMsg=2, QtFatalMsg=3)
    QVERIFY2(m_captured.contains(QString::number(static_cast<int>(QtWarningMsg)) + ":warn1"),
             "qWarning message should be captured at QtWarningMsg level");
    QVERIFY2(m_captured.contains(QString::number(static_cast<int>(QtCriticalMsg)) + ":crit1"),
             "qCritical message should be captured at QtCriticalMsg level");
    QVERIFY2(m_captured.contains(QString::number(static_cast<int>(QtDebugMsg)) + ":debug1"),
             "qDebug message should be captured at QtDebugMsg level");
}