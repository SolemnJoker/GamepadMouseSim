#pragma once

#include "core/Types.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>

class Config;

// Full configuration GUI. On accept(), writes all fields back to Config, which
// triggers the existing hot-reload pipeline (QFileSystemWatcher -> configChanged).
class SettingsDialog : public QDialog {
    Q_OBJECT
  public:
    explicit SettingsDialog(Config* config, QWidget* parent = nullptr);

  private slots:
    void accept() override;

  private:
    void buildGeneralTab();
    void buildStickTab();
    void buildMappingTab();
    void buildAutoSwitchTab();

    void loadValues();
    void saveValues();

    // Populate a mapping combo with the full ButtonAction set, selecting `current`.
    void fillActionCombo(QComboBox* combo, const QString& currentAction);
    QString comboAction(const QComboBox* combo) const;

    Config* m_config;

    QTabWidget* m_tabs = nullptr;

    // --- General tab ---
    QSpinBox* m_lockoutSec = nullptr;
    QListWidget* m_comboButtons = nullptr; // multi-select of 14 buttons
    QSpinBox* m_holdMs = nullptr;
    QCheckBox* m_osdEnabled = nullptr;
    QSpinBox* m_osdDuration = nullptr;
    QCheckBox* m_autostart = nullptr;

    // --- Stick tab ---
    QDoubleSpinBox* m_sensX = nullptr;
    QDoubleSpinBox* m_sensY = nullptr;
    QDoubleSpinBox* m_leftDeadzone = nullptr;
    QCheckBox* m_acceleration = nullptr;
    QDoubleSpinBox* m_scrollV = nullptr;
    QDoubleSpinBox* m_scrollH = nullptr;
    QDoubleSpinBox* m_rightDeadzone = nullptr;

    // --- Mapping tab ---
    // key = "button_mapping.<BtnName>", value = combo
    QMap<QString, QComboBox*> m_directCombos;
    QMap<QString, QComboBox*> m_l3Combos;
    QMap<QString, QComboBox*> m_rtCombos;

    // --- Auto-switch tab ---
    QCheckBox* m_asEnabled = nullptr;
    QSpinBox* m_asInterval = nullptr;
    QCheckBox* m_procEnabled = nullptr;
    QPlainTextEdit* m_procNames = nullptr;
    QCheckBox* m_fsEnabled = nullptr;
    QCheckBox* m_cpuEnabled = nullptr;
    QDoubleSpinBox* m_cpuThreshold = nullptr;
    QSpinBox* m_cpuSustained = nullptr;
    QCheckBox* m_gpuEnabled = nullptr;
    QDoubleSpinBox* m_gpuThreshold = nullptr;
    QSpinBox* m_gpuSustained = nullptr;
    QLabel* m_gpuHint = nullptr;
};
