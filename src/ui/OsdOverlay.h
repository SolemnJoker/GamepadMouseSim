#pragma once

#include "core/Types.h"
#include "ui/HelpContent.h"
#include <QPixmap>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVector>
#include <QWidget>

class Config;

class OsdOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float windowOpacity READ windowOpacity WRITE setWindowOpacity)
  public:
    explicit OsdOverlay(Config* config, QWidget* parent = nullptr);

  public slots:
    void showModeChange(int controllerIndex, GamepadMode mode);
    void showHelp();

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    void showMessage(const QString& text, int durationMs = 2000, int width = 300, int height = 60);
    void drawHelp(QPainter& painter, const QRect& rect) const;

    bool m_isHelpMode = false;
    QString m_text;
    Config* m_config = nullptr;
    QVector<HelpSection> m_helpSections;
    QPropertyAnimation m_fadeAnimation;
    QTimer m_hideTimer;
};
