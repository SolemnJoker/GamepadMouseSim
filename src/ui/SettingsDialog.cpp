#include "SettingsDialog.h"
#include "core/Config.h"
#include "core/Translations.h"
#include "core/MappingDefaults.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
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
                                                   ButtonAction::ShowHelp,
                                                   ButtonAction::ShowKeyboard};

SettingsDialog::SettingsDialog(Config* config, QWidget* parent)
    : QDialog(parent), m_config(config) {
    setWindowTitle(Translations::tr("设置"));
    setMinimumSize(560, 520);

    auto* root = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs);

    buildGeneralTab();
    buildStickTab();
    buildMappingTab();
    buildAutoSwitchTab();

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(Translations::tr("确定"));
    btns->button(QDialogButtonBox::Cancel)->setText(Translations::tr("取消"));
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
    m_lockoutSec->setSuffix(Translations::tr(" 秒"));
    form->addRow(Translations::tr("手动切换冷却时间:"), m_lockoutSec);

    // Combo key buttons: multi-select list
    auto* comboGroup = new QGroupBox(Translations::tr("模式切换组合键"), this);
    auto* cgLayout = new QVBoxLayout(comboGroup);
    m_comboButtons = new QListWidget(this);
    m_comboButtons->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const QString& b : kAllButtons) {
        m_comboButtons->addItem(b);
    }
    m_comboButtons->setMaximumHeight(160);
    cgLayout->addWidget(
        new QLabel(Translations::tr("按住以下按键切换模式（可多选，通常选 2 个）:"), this));
    cgLayout->addWidget(m_comboButtons);

    m_holdMs = new QSpinBox(this);
    m_holdMs->setRange(100, 5000);
    m_holdMs->setSingleStep(100);
    m_holdMs->setSuffix(Translations::tr(" 毫秒"));
    cgLayout->addWidget(new QLabel(Translations::tr("长按持续时间:"), this));
    cgLayout->addWidget(m_holdMs);
    form->addRow(comboGroup);

    auto* osdGroup = new QGroupBox(Translations::tr("OSD 通知"), this);
    auto* ogLayout = new QVBoxLayout(osdGroup);
    m_osdEnabled = new QCheckBox(Translations::tr("启用 OSD 通知"), this);
    ogLayout->addWidget(m_osdEnabled);
    m_osdDuration = new QSpinBox(this);
    m_osdDuration->setRange(1, 10);
    m_osdDuration->setSuffix(Translations::tr(" 秒"));
    ogLayout->addWidget(new QLabel(Translations::tr("OSD 显示时长:"), this));
    ogLayout->addWidget(m_osdDuration);
    form->addRow(osdGroup);

    m_autostart = new QCheckBox(Translations::tr("开机自动启动"), this);
    form->addRow(m_autostart);

    // 界面语言(design D3):选择即写入配置并热广播;对话框自身重开生效。
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem(Translations::tr("跟随系统"), "system");
    m_languageCombo->addItem(QStringLiteral("中文"), "zh");
    m_languageCombo->addItem(QStringLiteral("English"), "en");
    form->addRow(Translations::tr("界面语言:"), m_languageCombo);
    m_languageHint = new QLabel(Translations::tr("语言将在重新打开设置后完全生效。"), this);
    m_languageHint->setWordWrap(true);
    m_languageHint->setVisible(false);
    form->addRow(m_languageHint);
    connect(m_languageCombo, &QComboBox::activated, this, &SettingsDialog::onLanguageChanged);

    m_tabs->addTab(page, Translations::tr("常规"));
}

// ============================================================
// Tab 2: Stick sensitivity
// ============================================================
void SettingsDialog::buildStickTab() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);

    auto* leftGroup = new QGroupBox(Translations::tr("左摇杆（移动鼠标）"), this);
    auto* lg = new QFormLayout(leftGroup);
    m_sensX = new QDoubleSpinBox(this);
    m_sensX->setRange(0.1, 10.0);
    m_sensX->setSingleStep(0.1);
    lg->addRow(Translations::tr("X 灵敏度:"), m_sensX);
    m_sensY = new QDoubleSpinBox(this);
    m_sensY->setRange(0.1, 10.0);
    m_sensY->setSingleStep(0.1);
    lg->addRow(Translations::tr("Y 灵敏度:"), m_sensY);
    m_leftDeadzone = new QDoubleSpinBox(this);
    m_leftDeadzone->setRange(0.0, 0.5);
    m_leftDeadzone->setSingleStep(0.01);
    lg->addRow(Translations::tr("死区:"), m_leftDeadzone);
    m_acceleration = new QCheckBox(Translations::tr("启用鼠标加速"), this);
    lg->addRow(m_acceleration);
    form->addRow(leftGroup);

    auto* rightGroup = new QGroupBox(Translations::tr("右摇杆（滚动）"), this);
    auto* rg = new QFormLayout(rightGroup);
    m_scrollV = new QDoubleSpinBox(this);
    m_scrollV->setRange(0.1, 10.0);
    m_scrollV->setSingleStep(0.1);
    rg->addRow(Translations::tr("垂直滚动速度:"), m_scrollV);
    m_scrollH = new QDoubleSpinBox(this);
    m_scrollH->setRange(0.1, 10.0);
    m_scrollH->setSingleStep(0.1);
    rg->addRow(Translations::tr("水平滚动速度:"), m_scrollH);
    m_rightDeadzone = new QDoubleSpinBox(this);
    m_rightDeadzone->setRange(0.0, 0.5);
    m_rightDeadzone->setSingleStep(0.01);
    rg->addRow(Translations::tr("死区:"), m_rightDeadzone);
    form->addRow(rightGroup);

    m_tabs->addTab(page, Translations::tr("摇杆灵敏度"));
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

    // Profile bar:选择生效方案 + 管理(设计 D6)。
    auto* profileGroup = new QGroupBox(Translations::tr("操作方案(Profile)"), this);
    auto* pLayout = new QHBoxLayout(profileGroup);
    pLayout->addWidget(new QLabel(Translations::tr("当前方案:"), this));
    m_profileCombo = new QComboBox(this);
    pLayout->addWidget(m_profileCombo, 1);
    auto* createBtn = new QPushButton(Translations::tr("新建"), this);
    auto* renameBtn = new QPushButton(Translations::tr("重命名"), this);
    auto* deleteBtn = new QPushButton(Translations::tr("删除"), this);
    auto* resetBtn = new QPushButton(Translations::tr("恢复默认"), this);
    pLayout->addWidget(createBtn);
    pLayout->addWidget(renameBtn);
    pLayout->addWidget(deleteBtn);
    pLayout->addWidget(resetBtn);
    outer->addWidget(profileGroup);

    connect(m_profileCombo, &QComboBox::activated, this, &SettingsDialog::onProfileSelected);
    connect(createBtn, &QPushButton::clicked, this, &SettingsDialog::onProfileCreate);
    connect(renameBtn, &QPushButton::clicked, this, &SettingsDialog::onProfileRename);
    connect(deleteBtn, &QPushButton::clicked, this, &SettingsDialog::onProfileDelete);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetProfileDefaults);

    auto* directGroup = new QGroupBox(Translations::tr("直接映射"), this);
    auto* dg = new QFormLayout(directGroup);
    // Direct mapping applies to all 16 logical buttons.
    addMappingRow(dg, kAllButtons, m_directCombos, this,
                  [this](QComboBox* c, const QString& cur) { fillActionCombo(c, cur); });
    outer->addWidget(directGroup);

    auto* l3Group = new QGroupBox(Translations::tr("L3 层（按住 L3）"), this);
    auto* l3g = new QFormLayout(l3Group);
    // L3 layer buttons: all except L3 itself
    QStringList l3Btns = kAllButtons;
    l3Btns.removeAll("L3");
    addMappingRow(l3g, l3Btns, m_l3Combos, this,
                  [this](QComboBox* c, const QString& cur) { fillActionCombo(c, cur); });
    outer->addWidget(l3Group);

    auto* rtGroup = new QGroupBox(Translations::tr("RT 层（按住 RT）"), this);
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

    m_tabs->addTab(scroll, Translations::tr("按键映射"));
}

// ============================================================
// Tab 4: Auto-switch
// ============================================================
void SettingsDialog::buildAutoSwitchTab() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);

    m_asEnabled = new QCheckBox(Translations::tr("启用自动切换模式"), this);
    form->addRow(m_asEnabled);

    m_asInterval = new QSpinBox(this);
    m_asInterval->setRange(1, 300);
    m_asInterval->setSuffix(Translations::tr(" 秒"));
    form->addRow(Translations::tr("检测间隔:"), m_asInterval);

    auto* info = new QLabel(
        Translations::tr("规则：仅在<b>鼠标模式</b>下检测；检测到玩游戏时自动切到<b>默认模式</b>。"
                         "<b>默认模式</b>不会被自动改变，需手动切回鼠标模式。"),
        this);
    info->setWordWrap(true);
    form->addRow(info);

    auto* procGroup = new QGroupBox(Translations::tr("游戏进程检测"), this);
    auto* pg = new QVBoxLayout(procGroup);
    m_procEnabled =
        new QCheckBox(Translations::tr("启用（命中配置的游戏进程即视为在玩游戏）"), this);
    pg->addWidget(m_procEnabled);
    pg->addWidget(new QLabel(Translations::tr("游戏进程名（每行一个，例如 game.exe）:"), this));
    m_procNames = new QPlainTextEdit(this);
    m_procNames->setMaximumHeight(120);
    pg->addWidget(m_procNames);
    form->addRow(procGroup);

    auto* fsGroup = new QGroupBox(Translations::tr("全屏窗口检测"), this);
    auto* fsg = new QVBoxLayout(fsGroup);
    m_fsEnabled = new QCheckBox(Translations::tr("前台窗口全屏时视为在玩游戏"), this);
    fsg->addWidget(m_fsEnabled);
    form->addRow(fsGroup);

    auto* cpuGroup = new QGroupBox(Translations::tr("CPU 占用检测"), this);
    auto* cpg = new QFormLayout(cpuGroup);
    m_cpuEnabled = new QCheckBox(Translations::tr("启用"), this);
    cpg->addRow(m_cpuEnabled);
    m_cpuThreshold = new QDoubleSpinBox(this);
    m_cpuThreshold->setRange(1.0, 99.0);
    m_cpuThreshold->setSuffix(QStringLiteral(" %"));
    cpg->addRow(Translations::tr("CPU 占用阈值:"), m_cpuThreshold);
    m_cpuSustained = new QSpinBox(this);
    m_cpuSustained->setRange(1, 600);
    m_cpuSustained->setSuffix(Translations::tr(" 秒"));
    cpg->addRow(Translations::tr("持续时长（超过此时长才触发）:"), m_cpuSustained);
    form->addRow(cpuGroup);

    auto* gpuGroup = new QGroupBox(Translations::tr("GPU 占用检测（近似值）"), this);
    auto* gpg = new QFormLayout(gpuGroup);
    m_gpuEnabled = new QCheckBox(Translations::tr("启用"), this);
    gpg->addRow(m_gpuEnabled);
    m_gpuThreshold = new QDoubleSpinBox(this);
    m_gpuThreshold->setRange(1.0, 99.0);
    m_gpuThreshold->setSuffix(QStringLiteral(" %"));
    gpg->addRow(Translations::tr("GPU 占用阈值:"), m_gpuThreshold);
    m_gpuSustained = new QSpinBox(this);
    m_gpuSustained->setRange(1, 600);
    m_gpuSustained->setSuffix(Translations::tr(" 秒"));
    gpg->addRow(Translations::tr("持续时长:"), m_gpuSustained);
    m_gpuHint = new QLabel(
        Translations::tr("提示：GPU 占用为近似值，依赖驱动支持；不同硬件可能不可用。"), this);
    m_gpuHint->setWordWrap(true);
    gpg->addRow(m_gpuHint);
    form->addRow(gpuGroup);

    m_tabs->addTab(page, Translations::tr("自动切换"));
}

// ============================================================
// Load / Save
// ============================================================
void SettingsDialog::loadValues() {
    // Profiles
    reloadProfileCombo();

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
    {
        const int idx = m_languageCombo->findData(
            m_config->value("ui.language", QStringLiteral("system")).toString());
        m_languageCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        m_languageHint->setVisible(false);
    }

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

    // Mappings - merged view(共享默认表 ⊕ 配置,design D2/D6):缺失键
    // 显示默认动作,保存所见即所得,不再显示整页"无"。
    const auto direct = MappingDefaults::mergedDirect(
        m_config->value("mouse_mode.button_mapping").toJsonValue().toObject());
    for (auto it = m_directCombos.begin(); it != m_directCombos.end(); ++it) {
        fillActionCombo(it.value(), actionToString(direct.value(it.key())));
    }
    const QJsonObject modObj =
        m_config->value("mouse_mode.modifier_mapping").toJsonValue().toObject();
    const auto l3Layer = MappingDefaults::mergedLayer("LT", modObj);
    for (auto it = m_l3Combos.begin(); it != m_l3Combos.end(); ++it) {
        fillActionCombo(it.value(), actionToString(l3Layer.value(it.key())));
    }
    const auto rtLayer = MappingDefaults::mergedLayer("RT", modObj);
    for (auto it = m_rtCombos.begin(); it != m_rtCombos.end(); ++it) {
        fillActionCombo(it.value(), actionToString(rtLayer.value(it.key())));
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
    // Write under the runtime layer name "LT" (see loadValues note above).
    m_config->setValue("mouse_mode.modifier_mapping.LT", l3);

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

    // Keep the active profile's stored copy in sync with what we just wrote
    // to mouse_mode.* (design D3 mirror discipline).
    m_config->updateActiveProfileFromMouseMode();

    m_config->endBatch();
}

void SettingsDialog::reloadProfileCombo() {
    if (!m_profileCombo)
        return;
    m_profileComboUpdating = true;
    m_profileCombo->clear();
    m_profileCombo->addItems(m_config->profileOrder());
    const int idx = m_profileCombo->findText(m_config->activeProfileName());
    if (idx >= 0)
        m_profileCombo->setCurrentIndex(idx);
    m_profileComboUpdating = false;
}

void SettingsDialog::onProfileSelected() {
    if (m_profileComboUpdating)
        return;
    const QString name = m_profileCombo->currentText();
    if (name.isEmpty() || name == m_config->activeProfileName())
        return;
    m_config->setActiveProfile(name); // 展开 → configChanged 热广播
    loadValues();                     // 显示新方案的合并视图
}

void SettingsDialog::onProfileCreate() {
    bool ok = false;
    QString name =
        QInputDialog::getText(this, Translations::tr("新建操作方案"), Translations::tr("方案名称:"),
                              QLineEdit::Normal, Translations::tr("新方案"), &ok);
    if (!ok)
        return;
    name = name.trimmed();
    if (!m_config->createProfile(name)) {
        QMessageBox::warning(this, Translations::tr("新建操作方案"),
                             Translations::tr("无法创建:名称为空或已存在。"));
        return;
    }
    m_config->setActiveProfile(name);
    reloadProfileCombo();
}

void SettingsDialog::onProfileRename() {
    const QString oldName = m_config->activeProfileName();
    bool ok = false;
    QString name =
        QInputDialog::getText(this, Translations::tr("重命名操作方案"), Translations::tr("新名称:"),
                              QLineEdit::Normal, oldName, &ok);
    if (!ok)
        return;
    name = name.trimmed();
    if (!m_config->renameProfile(oldName, name)) {
        QMessageBox::warning(this, Translations::tr("重命名操作方案"),
                             Translations::tr("无法重命名:新名称为空或已存在。"));
        return;
    }
    reloadProfileCombo();
}

void SettingsDialog::onProfileDelete() {
    const QString name = m_config->activeProfileName();
    if (QMessageBox::question(this, Translations::tr("删除操作方案"),
                              Translations::tr("删除方案 \"%1\"?").arg(name)) != QMessageBox::Yes)
        return;
    if (!m_config->removeProfile(name)) {
        QMessageBox::warning(this, Translations::tr("删除操作方案"),
                             Translations::tr("至少需要保留一套方案。"));
        return;
    }
    reloadProfileCombo();
    loadValues();
}

void SettingsDialog::onResetProfileDefaults() {
    const QString name = m_config->activeProfileName();
    if (QMessageBox::question(this, Translations::tr("恢复默认"),
                              Translations::tr("将方案 \"%1\" 的全部映射恢复为默认?").arg(name)) !=
        QMessageBox::Yes)
        return;
    m_config->beginBatch();
    m_config->setValue("mouse_mode.button_mapping", MappingDefaults::directDefaultsJson());
    QJsonObject mods;
    mods["LT"] = MappingDefaults::ltLayerDefaultsJson();
    mods["RT"] = MappingDefaults::rtLayerDefaultsJson();
    m_config->setValue("mouse_mode.modifier_mapping", mods);
    m_config->endBatch();
    m_config->updateActiveProfileFromMouseMode();
    loadValues();
}

void SettingsDialog::onLanguageChanged() {
    m_config->setValue("ui.language", m_languageCombo->currentData().toString());
    // 提示用"当前(旧)语言"呈现——热广播 300ms 后才切换语言,对话框文案
    // 在重开后更新(spec: 设置对话框边界)。
    m_languageHint->setText(Translations::tr("语言将在重新打开设置后完全生效。"));
    m_languageHint->setVisible(true);
}

void SettingsDialog::accept() {
    saveValues();
    QDialog::accept();
}
