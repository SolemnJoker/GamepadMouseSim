#pragma once

#include <QtTest>
#include "core/Types.h"

class TestActionToChinese : public QObject {
    Q_OBJECT
  private slots:
    void allActions_haveNonEmptyMapping();
};