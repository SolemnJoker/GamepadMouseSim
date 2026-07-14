#pragma once

#include <QString>
#include <QtTest>

class TestLogSeverity : public QObject {
    Q_OBJECT
  private slots:
    void init();
    void cleanup();

    void qWarning_capturesMessage();
    void qCritical_capturesMessage();
    void severityLevels_correctlyLabelled();

  private:
    QString m_captured;
    QtMessageHandler m_oldHandler = nullptr;

    static void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
    static TestLogSeverity* s_instance;
};