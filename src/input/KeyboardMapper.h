#pragma once

#include <QObject>
#include <QMap>
#include <QSet>
#include "core/Types.h"

class Config;

class KeyboardMapper : public QObject {
    Q_OBJECT
public:
    explicit KeyboardMapper(Config* config, QObject* parent = nullptr);

    void processButton(uint16_t button, bool pressed, uint16_t prevButtons);
    void processTrigger(float leftTrigger, float rightTrigger,
                        float prevLeftTrigger, float prevRightTrigger);
    void releaseModifiers();

    QStringList buildHelpLines() const;

public slots:
    void onConfigChanged();

signals:
    void scrollRequested(float dx, float dy);
    void showHelpRequested();

private:
    void loadConfig();
    void executeAction(ButtonAction action);

    ButtonAction lookupAction(const QString& btnName);

    Config* m_config;
    QMap<QString, ButtonAction> m_directMapping;
    QMap<QString, QMap<QString, ButtonAction>> m_modifierMapping;

    bool m_lbHeld = false;
    bool m_rbHeld = false;
    bool m_ltHeld = false;
    bool m_rtHeld = false;

    QSet<uint16_t> m_pressedButtons;
    QSet<uint16_t> m_ltTabBlocked;
    bool m_ltTabActive = false;
};
