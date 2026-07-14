#pragma once

#include <QObject>
#include <QStringList>

class ProcessDetector : public QObject {
    Q_OBJECT
  public:
    explicit ProcessDetector(QObject* parent = nullptr);
    bool isTargetRunning(const QStringList& processNames);
};
