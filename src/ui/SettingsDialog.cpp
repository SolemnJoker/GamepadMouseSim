#include "SettingsDialog.h"
#include "core/Config.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <functional>

// All editable button names (matches buttonBitToName() + the two triggers).
static const QStringList kAllButtons = {
    "A",  "B",  "X",  "Y",  "DpadUp", "DpadDown", "DpadLeft", "DpadRight",
    "LB", "RB", "LT", "RT", "L3",     "R3",       "View",     "Menu"};

// The full set of selectable actions, in display order.
static const QVector<ButtonAction> kActionOrder = {ButtonAction::None,
                                                   ButtonAction::MouseLeftClick,
                                                   ButtonAction::MouseRightClick,
                                                   ButtonAction::MouseMiddleClick,
                                                   ButtonAction::MouseLeftHold,
                                                   ButtonAction::MouseRightHold,
                                                   ButtonAction::KeyEnter,
                                                   ButtonAction::KeyEscape,
                                                   ButtonAction::KeyTab,
                                                   ButtonAction::KeyShiftTab,
                                                   ButtonAction::KeyAltTab,
                                                   ButtonAction::KeyAltF4,
                                                   ButtonAction::KeyWin,
                                                   ButtonAction::KeyWinD,
                                                   ButtonAction::KeyBackspace,
                                                   ButtonAction::KeyDelete,
                                                   ButtonAction::KeyHome,
                                                   ButtonAction::KeyEnd,
                                                   ButtonAction::KeyPageUp,
                                                   ButtonAction::KeyPageDown,
                                                   ButtonAction::KeyArrowUp,
                                                   ButtonAction::KeyArrowDown,
                                                   ButtonAction::KeyArrowLeft,
                                                   ButtonAction::KeyArrowRight,
                                                   ButtonAction::KeyCtrlW,
                                                   ButtonAction::KeyCtrlA,
                                                   ButtonAction::KeyCtrlC,
                                                   ButtonAction::KeyCtrlV,
                                                   ButtonAction::KeyCtrlX,
                                                   ButtonAction::KeyCtrlZ,
                                                   ButtonAction::KeyCtrlShiftZ,
                                                   ButtonAction::KeyCtrlS,
                                                   ButtonAction::KeyCtrlTab,
                                                   ButtonAction::KeyCtrlShiftTab,
                                                   ButtonAction::KeyF5,
                                                   ButtonAction::MediaPrevTrack,
                                                   ButtonAction::MediaNextTrack,
                                                   ButtonAction::VolumeUp,
                                                   ButtonAction::VolumeDown,
                                                   ButtonAction::VolumeMute,
                                                   ButtonAction::ScrollUp,
                                                   ButtonAction::ScrollDown,
                                                   ButtonAction::ScrollLeft,
                                                   ButtonAction::ScrollRight,
                                                   ButtonAction::ShowHelp};

SettingsDialog::SettingsDialog(Config* config, QWidget* parent)
    : QDialog(parent), m_config(config) {
    setWindowTitle(QStringLiteral("设置"));
    setMinimumSize(560, 520);

    auto* root = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs);

    buildGeneralTab();
    buildStickTab();
    buildMappingTab();
    buildAutoSwitchTab();

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    btns->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    loadValues();
}

void SettingsDialog::fillActionCombo(QComboBox* combo, const QString& currentAction) {
    combo->blockSignals(true);
    combo->clear();
    for (ButtonAction a : kActionOrder) {
        QString code = actionToString(a);
        combo->addItem(actionToChinese(a), code);
        if (code == currentAction) {
            combo->setCurrentIndex(combo->count() - 1);
        }
    }
    combo->blockSignals(false);
}

QString SettingsDialog::comboAction(const QComboBox* combo) const {
    return combo->currentData().toString();
}

// ============================================================
// Tab 1: General
// ============================================================
void SettingsDialog::buildGeneralTab() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);

    m_lockoutSec = new QSpinBox(this);
    m_lockoutSec->setRange(0, 60);
    m_lockoutSec->setSuffix(QStringLiteral(" 秒"));
    form->addRow(QStringLiteral("手动切换冷却时间:"), m_lockoutSec);

    // Combo key buttons: multi-select list
    auto* comboGroup = new QGroupBox(QStringLiteral("模式切换组合键"), this);
    auto* cgLayout = new QVBoxLayout(comboGroup);
    m_comboButtons = new QListWidget(this);
    m_comboButtons->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const QString& b : kAllButtons) {
        m_comboButtons->addItem(b);
    }
    m_comboButtons->setMaximumHeight(160);
    cgLayout->addWidget(
        new QLabel(QStringLiteral("按住以下按键切换模式（可多选，通常选 2 个）:"), this));
    cgLayout->addWidget(m_comboButtons);

    m_holdMs = new QSpinBox(this);
    m_holdMs->setRange(100, 5000);
    m_holdMs->setSingleStep(100);
    m_holdMs->setSuffix(QStringLiteral(" 毫秒"));
    cgLayout->addWidget(new QLabel(QStringLiteral("长按持续时间:"), this));
    cgLayout->addWidget(m_holdMs);
    form->addRow(comboGroup);

    auto* osdGroup = new QGroupBox(QStringLiteral("OSD 通知"), this);
    auto* ogLayout = new QVBoxLayout(osdGroup);
    m_osdEnabled = new QCheckBox(QStringLiteral("启用 OSD 通知"), this);
    ogLayout->addWidget(m_osdEnabled);
    m_osdDuration = new QSpinBox(this);
    m_osdDuration->setRange(1, 10);
    m_osdDuration->setSuffix(QStringLiteral(" 秒"));
    ogLayout->addWidget(new QLabel(QStringLiteral("OSD 显示时长:"), this));
    ogLayout->addWidget(m_osdDuration);
    form->addRow(osdGroup);

    m_autostart = new QCheckBox(QStringLiteral("开机自动启动"), this);
    form->addRow(m_autostart);

    m_tabs->addTab(page, QStringLiteral("常规"));
}

// ============================================================
// Tab 2: Stick sensitivity
// ============================================================
void SettingsDialog::buildStickTab() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);

    auto* leftGroup = new QGroupBox(QStringLiteral("左摇杆（移动鼠标）"), this);
    auto* lg = new QFormLayout(leftGroup);
    m_sensX = new QDoubleSpinBox(this);
    m_sensX->setRange(0.1, 10.0);
    m_sensX->setSingleStep(0.1);
    lg->addRow(QStringLiteral("X 灵敏度:"), m_sensX);
    m_sensY = new QDoubleSpinBox(this);
    m_sensY->setRange(0.1, 10.0);
    m_sensY->setSingleStep(0.1);
    lg->addRow(QStringLiteral("Y 灵敏度:"), m_sensY);
    m_leftDeadzone = new QDoubleSpinBox(this);
    m_leftDeadzone->setRange(0.0, 0.5);
    m_leftDeadzone->setSingleStep(0.01);
    lg->addRow(QStringLiteral("死区:"), m_leftDeadzone);
    m_acceleration = new QCheckBox(QStringLiteral("启用鼠标加速"), this);
    lg->addRow(m_acceleration);
    form->addRow(leftGroup);

    auto* rightGroup = new QGroupBox(QStringLiteral("右摇杆（滚动）"), this);
    auto* rg = new QFormLayout(rightGroup);
    m_scrollV = new QDoubleSpinBox(this);
    m_scrollV->setRange(0.1, 10.0);
    m_scrollV->setSingleStep(0.1);
    rg->addRow(QStringLiteral("垂直滚动速度:"), m_scrollV);
    m_scrollH = new QDoubleSpinBox(this);
    m_scrollH->setRange(0.1, 10.0);
    m_scrollH->setSingleStep(0.1);
    rg->addRow(QStringLiteral("水平滚动速度:"), m_scrollH);
    m_rightDeadzone = new QDoubleSpinBox(this);
    m_rightDeadzone->setRange(0.0, 0.5);
    m_rightDeadzone->setSingleStep(0.01);
    rg->addRow(QStringLiteral("死区:"), m_rightDeadzone);
    form->addRow(rightGroup);

    m_tabs->addTab(page, QStringLiteral("摇杆灵敏度"));
}

// ============================================================
// Tab 3: Button mappings
// ============================================================
static void addMappingRow(QFormLayout* form, const QStringList& buttons,
                          QMap<QString, QComboBox*>& store, SettingsDialog* dlg,
                          const std::function<void(QComboBox*, const QString&)>& filler) {
    for (const QString& b : buttons) {
        auto* combo = new QComboBox(form->parentWidget());
        filler(combo, "None");
        store.insert(b, combo);
        form->addRow(b + ":", combo);
    }
}

void SettingsDialog::buildMappingTab() {
    auto* page = new QWidget(this);
    auto* outer = new QVBoxLayout(page);

    auto* directGroup = new QGroupBox(QStringLiteral("直接映射"), this);
    auto* dg = new QFormLayout(directGroup);
    // Direct mapping applies to all 16 logical buttons.
    addMappingRow(dg, kAllButtons, m_directCombos, this,
                  [this](QComboBox* c, const QString& cur) { fillActionCombo(c, cur); });
    outer->addWidget(directGroup);

    auto* l3Group = new QGroupBox(QStringLiteral("L3 层（按住 L3）"), this);
    auto* l3g = new QFormLayout(l3Group);
    // L3 layer buttons: all except L3 itself
    QStringList l3Btns = kAllButtons;
    l3Btns.removeAll("L3");
    addMappingRow(l3g, l3Btns, m_l3Combos, this,
                  [this](QComboBox* c, const QString& cur) { fillActionCombo(c, cur); });
    outer->addWidget(l3Group);

    auto* rtGroup = new QGroupBox(QStringLiteral("RT 层（按住 RT）"), this);
    auto* rtg = new QFormLayout(rtGroup);
    QStringList rtBtns = kAllButtons;
    rtBtns.removeAll("RT");
    addMappingRow(rtg, rtBtns, m_rtCombos, this,
                  [this](QComboBox* c, const QString& cur) { fillActionCombo(c, cur); });
    outer->addWidget(rtGroup);

    outer->addStretch();
    page->setLayout(outer);

    // Put it in a scroll area since it's long.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(page);

    m_tabs->addTab(scroll, QStringLiteral("按键映射"));
}

// ============================================================
// Tab 4: Auto-switch
// ============================================================
void SettingsDialog::buildAutoSwitchTab() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);

    m_asEnabled = new QCheckBox(QStringLiteral("启用自动切换模式"), this);
    form->addRow(m_asEnabled);

    m_asInterval = new QSpinBox(this);
    m_asInterval->setRange(1, 300);
    m_asInterval->setSuffix(QStringLiteral(" 秒"));
    form->addRow(QStringLiteral("检测间隔:"), m_asInterval);

    auto* info = new QLabel(
        QStringLiteral("规则：仅在<b>鼠标模式</b>下检测；检测到玩游戏时自动切到<b>默认模式</b>。"
                       "<b>默认模式</b>不会被自动改变，需手动切回鼠标模式。"),
        this);
    info->setWordWrap(true);
    form->addRow(info);

    auto* procGroup = new QGroupBox(QStringLiteral("游戏进程检测"), this);
    auto* pg = new QVBoxLayout(procGroup);
    m_procEnabled = new QCheckBox(QStringLiteral("启用（命中配置的游戏进程即视为在玩游戏）"), this);
    pg->addWidget(m_procEnabled);
    pg->addWidget(new QLabel(QStringLiteral("游戏进程名（每行一个，例如 game.exe）:"), this));
    m_procNames = new QPlainTextEdit(this);
    m_procNames->setMaximumHeight(120);
    pg->addWidget(m_procNames);
    form->addRow(procGroup);

    auto* fsGroup = new QGroupBox(QStringLiteral("全屏窗口检测"), this);
    auto* fsg = new QVBoxLayout(fsGroup);
    m_fsEnabled = new QCheckBox(QStringLiteral("前台窗口全屏时视为在玩游戏"), this);
    fsg->addWidget(m_fsEnabled);
    form->addRow(fsGroup);

    auto* cpuGroup = new QGroupBox(QStringLiteral("CPU 占用检测"), this);
    auto* cpg = new QFormLayout(cpuGroup);
    m_cpuEnabled = new QCheckBox(QStringLiteral("启用"), this);
    cpg->addRow(m_cpuEnabled);
    m_cpuThreshold = new QDoubleSpinBox(this);
    m_cpuThreshold->setRange(1.0, 99.0);
    m_cpuThreshold->setSuffix(QStringLiteral(" %"));
    cpg->addRow(QStringLiteral("CPU 占用阈值:"), m_cpuThreshold);
    m_cpuSustained = new QSpinBox(this);
    m_cpuSustained->setRange(1, 600);
    m_cpuSustained->setSuffix(QStringLiteral(" 秒"));
    cpg->addRow(QStringLiteral("持续时长（超过此时长才触发）:"), m_cpuSustained);
    form->addRow(cpuGroup);

    auto* gpuGroup = new QGroupBox(QStringLiteral("GPU 占用检测（近似值）"), this);
    auto* gpg = new QFormLayout(gpuGroup);
    m_gpuEnabled = new QCheckBox(QStringLiteral("启用"), this);
    gpg->addRow(m_gpuEnabled);
    m_gpuThreshold = new QDoubleSpinBox(this);
    m_gpuThreshold->setRange(1.0, 99.0);
    m_gpuThreshold->setSuffix(QStringLiteral(" %"));
    gpg->addRow(QStringLiteral("GPU 占用阈值:"), m_gpuThreshold);
    m_gpuSustained = new QSpinBox(this);
    m_gpuSustained->setRange(1, 600);
    m_gpuSustained->setSuffix(QStringLiteral(" 秒"));
    gpg->addRow(QStringLiteral("持续时长:"), m_gpuSustained);
    m_gpuHint = new QLabel(
        QStringLiteral("提示：GPU 占用为近似值，依赖驱动支持；不同硬件可能不可用。"), this);
    m_gpuHint->setWordWrap(true);
    gpg->addRow(m_gpuHint);
    form->addRow(gpuGroup);

    m_tabs->addTab(page, QStringLiteral("自动切换"));
}

// ============================================================
// Load / Save
// ============================================================
void SettingsDialog::loadValues() {
    // General
    m_lockoutSec->setValue(m_config->value("monitoring.manual_switch_lockout_seconds", 3).toInt());
    m_holdMs->setValue(m_config->value("combo_key.hold_duration_ms", 1000).toInt());
    QVariant comboV = m_config->value("combo_key.buttons");
    QStringList comboBtns;
    if (comboV.canConvert<QVariantList>()) {
        for (const QVariant& item : comboV.toList()) {
            comboBtns << item.toString();
        }
    }
    for (int i = 0; i < m_comboButtons->count(); ++i) {
        QListWidgetItem* item = m_comboButtons->item(i);
        item->setSelected(comboBtns.contains(item->text()));
    }
    m_osdEnabled->setChecked(m_config->value("osd.enabled", true).toBool());
    m_osdDuration->setValue(m_config->value("osd.duration_seconds", 2).toInt());
    m_autostart->setChecked(m_config->value("autostart", false).toBool());

    // Stick
    m_sensX->setValue(m_config->value("mouse_mode.left_stick.sensitivity_x", 1.0).toDouble());
    m_sensY->setValue(m_config->value("mouse_mode.left_stick.sensitivity_y", 1.0).toDouble());
    m_leftDeadzone->setValue(m_config->value("mouse_mode.left_stick.deadzone", 0.15).toDouble());
    m_acceleration->setChecked(
        m_config->value("mouse_mode.left_stick.acceleration", true).toBool());
    m_scrollV->setValue(
        m_config->value("mouse_mode.right_stick.scroll_speed_vertical", 1.0).toDouble());
    m_scrollH->setValue(
        m_config->value("mouse_mode.right_stick.scroll_speed_horizontal", 1.0).toDouble());
    m_rightDeadzone->setValue(m_config->value("mouse_mode.right_stick.deadzone", 0.15).toDouble());

    // Mappings - direct
    QJsonObject direct = m_config->value("mouse_mode.button_mapping").toJsonObject();
    for (auto it = m_directCombos.begin(); it != m_directCombos.end(); ++it) {
        QString cur = direct.value(it.key()).toString("None");
        fillActionCombo(it.value(), cur);
    }
    QJsonObject l3Obj = m_config->value("mouse_mode.modifier_mapping.L3").toJsonObject();
    for (auto it = m_l3Combos.begin(); it != m_l3Combos.end(); ++it) {
        QString cur = l3Obj.value(it.key()).toString("None");
        fillActionCombo(it.value(), cur);
    }
    QJsonObject rtObj = m_config->value("mouse_mode.modifier_mapping.RT").toJsonObject();
    for (auto it = m_rtCombos.begin(); it != m_rtCombos.end(); ++it) {
        QString cur = rtObj.value(it.key()).toString("None");
        fillActionCombo(it.value(), cur);
    }

    // Auto-switch
    m_asEnabled->setChecked(m_config->value("auto_switch.enabled", false).toBool());
    m_asInterval->setValue(m_config->value("auto_switch.poll_interval_seconds", 10).toInt());
    m_procEnabled->setChecked(
        m_config->value("auto_switch.detection.process_list_enabled", true).toBool());
    m_procNames->setPlainText(([&]() {
        QVariant v = m_config->value("auto_switch.detection.process_names");
        QStringList out;
        if (v.canConvert<QVariantList>()) {
            for (const QVariant& item : v.toList()) {
                if (!item.toString().trimmed().isEmpty())
                    out << item.toString().trimmed();
            }
        }
        return out.join("\n");
    })());
    m_fsEnabled->setChecked(
        m_config->value("auto_switch.detection.fullscreen_enabled", false).toBool());
    m_cpuEnabled->setChecked(m_config->value("auto_switch.detection.cpu_enabled", false).toBool());
    m_cpuThreshold->setValue(m_config->value("auto_switch.detection.cpu_threshold", 50).toDouble());
    m_cpuSustained->setValue(
        m_config->value("auto_switch.detection.cpu_sustained_seconds", 30).toInt());
    m_gpuEnabled->setChecked(m_config->value("auto_switch.detection.gpu_enabled", false).toBool());
    m_gpuThreshold->setValue(m_config->value("auto_switch.detection.gpu_threshold", 50).toDouble());
    m_gpuSustained->setValue(
        m_config->value("auto_switch.detection.gpu_sustained_seconds", 30).toInt());
}

void SettingsDialog::saveValues() {
    m_config->beginBatch();

    // General
    m_config->setValue("monitoring.manual_switch_lockout_seconds", m_lockoutSec->value());
    m_config->setValue("combo_key.hold_duration_ms", m_holdMs->value());
    QStringList selectedBtns;
    for (QListWidgetItem* item : m_comboButtons->selectedItems()) {
        selectedBtns << item->text();
    }
    m_config->setValue("combo_key.buttons", selectedBtns);
    m_config->setValue("osd.enabled", m_osdEnabled->isChecked());
    m_config->setValue("osd.duration_seconds", m_osdDuration->value());

    // Boot autostart: write through Config so configChanged fans out
    // to Application, which is the sole writer of the registry.
    m_config->setValue("autostart", m_autostart->isChecked());

    // Stick
    m_config->setValue("mouse_mode.left_stick.sensitivity_x", m_sensX->value());
    m_config->setValue("mouse_mode.left_stick.sensitivity_y", m_sensY->value());
    m_config->setValue("mouse_mode.left_stick.deadzone", m_leftDeadzone->value());
    m_config->setValue("mouse_mode.left_stick.acceleration", m_acceleration->isChecked());
    m_config->setValue("mouse_mode.right_stick.scroll_speed_vertical", m_scrollV->value());
    m_config->setValue("mouse_mode.right_stick.scroll_speed_horizontal", m_scrollH->value());
    m_config->setValue("mouse_mode.right_stick.deadzone", m_rightDeadzone->value());

    // Mappings
    QJsonObject direct;
    for (auto it = m_directCombos.begin(); it != m_directCombos.end(); ++it) {
        direct[it.key()] = comboAction(it.value());
    }
    m_config->setValue("mouse_mode.button_mapping", direct);

    QJsonObject l3;
    for (auto it = m_l3Combos.begin(); it != m_l3Combos.end(); ++it) {
        l3[it.key()] = comboAction(it.value());
    }
    m_config->setValue("mouse_mode.modifier_mapping.L3", l3);

    QJsonObject rt;
    for (auto it = m_rtCombos.begin(); it != m_rtCombos.end(); ++it) {
        rt[it.key()] = comboAction(it.value());
    }
    m_config->setValue("mouse_mode.modifier_mapping.RT", rt);

    // Auto-switch
    m_config->setValue("auto_switch.enabled", m_asEnabled->isChecked());
    m_config->setValue("auto_switch.poll_interval_seconds", m_asInterval->value());
    m_config->setValue("auto_switch.detection.process_list_enabled", m_procEnabled->isChecked());
    QStringList names;
    for (const QString& line : m_procNames->toPlainText().split('\n', Qt::SkipEmptyParts)) {
        QString t = line.trimmed();
        if (!t.isEmpty())
            names << t;
    }
    m_config->setValue("auto_switch.detection.process_names", names);
    m_config->setValue("auto_switch.detection.fullscreen_enabled", m_fsEnabled->isChecked());
    m_config->setValue("auto_switch.detection.cpu_enabled", m_cpuEnabled->isChecked());
    m_config->setValue("auto_switch.detection.cpu_threshold", m_cpuThreshold->value());
    m_config->setValue("auto_switch.detection.cpu_sustained_seconds", m_cpuSustained->value());
    m_config->setValue("auto_switch.detection.gpu_enabled", m_gpuEnabled->isChecked());
    m_config->setValue("auto_switch.detection.gpu_threshold", m_gpuThreshold->value());
    m_config->setValue("auto_switch.detection.gpu_sustained_seconds", m_gpuSustained->value());

    m_config->endBatch();
}

void SettingsDialog::accept() {
    saveValues();
    QDialog::accept();
}
