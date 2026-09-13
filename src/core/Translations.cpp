#include "Translations.h"
#include <QHash>
#include <QLocale>

namespace Translations {
namespace {

Language g_language = Language::Chinese;

// 英文表:key = 中文原文(与调用点 tr("...") 字面量逐字一致),
// value = 英文。分组的注释对应文案归属;与 key 相同的英文条目是
// 有意保留的自明项(如 "Tab"),使其同样通过源码扫描测试。
const QHash<QString, QString>& englishTable() {
    static const QHash<QString, QString> table = {
        // --- 按键动作名(Types.cpp actionToChinese) ---
        {QStringLiteral("左键单击"), QStringLiteral("Left click")},
        {QStringLiteral("右键单击"), QStringLiteral("Right click")},
        {QStringLiteral("中键单击"), QStringLiteral("Middle click")},
        {QStringLiteral("左键按住"), QStringLiteral("Left hold")},
        {QStringLiteral("右键按住"), QStringLiteral("Right hold")},
        {QStringLiteral("回车"), QStringLiteral("Enter")},
        {QStringLiteral("退出"), QStringLiteral("Escape")},
        {QStringLiteral("Tab"), QStringLiteral("Tab")},
        {QStringLiteral("退格"), QStringLiteral("Backspace")},
        {QStringLiteral("删除"), QStringLiteral("Delete")},
        {QStringLiteral("Home"), QStringLiteral("Home")},
        {QStringLiteral("End"), QStringLiteral("End")},
        {QStringLiteral("上翻页"), QStringLiteral("Page up")},
        {QStringLiteral("下翻页"), QStringLiteral("Page down")},
        {QStringLiteral("方向↑"), QStringLiteral("Arrow up")},
        {QStringLiteral("方向↓"), QStringLiteral("Arrow down")},
        {QStringLiteral("方向←"), QStringLiteral("Arrow left")},
        {QStringLiteral("方向→"), QStringLiteral("Arrow right")},
        {QStringLiteral("Shift+Tab"), QStringLiteral("Shift+Tab")},
        {QStringLiteral("Alt+Tab"), QStringLiteral("Alt+Tab")},
        {QStringLiteral("Alt+F4"), QStringLiteral("Alt+F4")},
        {QStringLiteral("显示桌面"), QStringLiteral("Show desktop")},
        {QStringLiteral("关闭标签"), QStringLiteral("Close tab")},
        {QStringLiteral("全选"), QStringLiteral("Select all")},
        {QStringLiteral("复制"), QStringLiteral("Copy")},
        {QStringLiteral("粘贴"), QStringLiteral("Paste")},
        {QStringLiteral("剪切"), QStringLiteral("Cut")},
        {QStringLiteral("撤销"), QStringLiteral("Undo")},
        {QStringLiteral("重做"), QStringLiteral("Redo")},
        {QStringLiteral("保存"), QStringLiteral("Save")},
        {QStringLiteral("Ctrl+←"), QStringLiteral("Ctrl+Left")},
        {QStringLiteral("Ctrl+→"), QStringLiteral("Ctrl+Right")},
        {QStringLiteral("Ctrl+Tab"), QStringLiteral("Ctrl+Tab")},
        {QStringLiteral("Ctrl+Shift+Tab"), QStringLiteral("Ctrl+Shift+Tab")},
        {QStringLiteral("刷新"), QStringLiteral("Refresh")},
        {QStringLiteral("上一曲"), QStringLiteral("Previous track")},
        {QStringLiteral("下一曲"), QStringLiteral("Next track")},
        {QStringLiteral("音量+"), QStringLiteral("Volume up")},
        {QStringLiteral("音量-"), QStringLiteral("Volume down")},
        {QStringLiteral("静音"), QStringLiteral("Mute")},
        {QStringLiteral("开始菜单"), QStringLiteral("Start menu")},
        {QStringLiteral("向上滚动"), QStringLiteral("Scroll up")},
        {QStringLiteral("向下滚动"), QStringLiteral("Scroll down")},
        {QStringLiteral("向左滚动"), QStringLiteral("Scroll left")},
        {QStringLiteral("向右滚动"), QStringLiteral("Scroll right")},
        {QStringLiteral("显示帮助"), QStringLiteral("Show help")},
        {QStringLiteral("虚拟键盘"), QStringLiteral("On-screen keyboard")},

        // --- 托盘(SystemTray) ---
        {QStringLiteral("模式: 默认"), QStringLiteral("Mode: Default")},
        {QStringLiteral("操作方案"), QStringLiteral("Profiles")},
        {QStringLiteral("恢复默认配置"), QStringLiteral("Restore default configuration")},
        {QStringLiteral("设置..."), QStringLiteral("Settings...")},
        {QStringLiteral("切换模式"), QStringLiteral("Switch mode")},
        {QStringLiteral("锁定模式"), QStringLiteral("Lock mode")},
        {QStringLiteral("暂停映射"), QStringLiteral("Pause mapping")},
        {QStringLiteral("退出"), QStringLiteral("Exit")},
        {QStringLiteral("(无)"), QStringLiteral("(none)")},

        // --- 帮助屏(HelpContent) ---
        {QStringLiteral("直接映射"), QStringLiteral("Direct mapping")},
        {QStringLiteral("方向键(直接)"), QStringLiteral("D-pad (direct)")},
        {QStringLiteral("L3层 (按住L3)"), QStringLiteral("L3 layer (hold L3)")},
        {QStringLiteral("RT层 (按住RT)"), QStringLiteral("RT layer (hold RT)")},
        {QStringLiteral("摇杆"), QStringLiteral("Sticks")},
        {QStringLiteral("移动鼠标"), QStringLiteral("Move cursor")},
        {QStringLiteral("滚动页面"), QStringLiteral("Scroll")},
        {QStringLiteral("模式与系统"), QStringLiteral("Modes & system")},
        {QStringLiteral("(长按1秒)"), QStringLiteral(" (hold 1s)")},
        {QStringLiteral("手柄鼠标模拟器"), QStringLiteral("Gamepad Mouse Simulator")},

        // --- 确认对话框(Application) ---
        {QStringLiteral("将丢弃全部自定义配置(含所有操作方案),恢复为出厂默认。确定继续?"),
         QStringLiteral("This will discard all custom settings (including every profile) and "
                        "restore factory defaults. Continue?")},

        // --- 设置界面(SettingsDialog) ---
        {QStringLiteral("设置"), QStringLiteral("Settings")},
        {QStringLiteral("确定"), QStringLiteral("OK")},
        {QStringLiteral("取消"), QStringLiteral("Cancel")},
        {QStringLiteral(" 秒"), QStringLiteral(" s")},
        {QStringLiteral(" 毫秒"), QStringLiteral(" ms")},
        {QStringLiteral("手动切换冷却时间:"), QStringLiteral("Manual switch cooldown:")},
        {QStringLiteral("模式切换组合键"), QStringLiteral("Mode-switch combo")},
        {QStringLiteral("按住以下按键切换模式（可多选，通常选 2 个）:"),
         QStringLiteral("Hold these buttons to switch mode (multi-select, usually 2):")},
        {QStringLiteral("长按持续时间:"), QStringLiteral("Hold duration:")},
        {QStringLiteral("OSD 通知"), QStringLiteral("OSD notifications")},
        {QStringLiteral("启用 OSD 通知"), QStringLiteral("Enable OSD notifications")},
        {QStringLiteral("OSD 显示时长:"), QStringLiteral("OSD duration:")},
        {QStringLiteral("开机自动启动"), QStringLiteral("Start on boot")},
        {QStringLiteral("界面语言:"), QStringLiteral("UI language:")},
        {QStringLiteral("跟随系统"), QStringLiteral("Follow system")},
        {QStringLiteral("语言将在重新打开设置后完全生效。"),
         QStringLiteral("Language changes fully apply after reopening the settings.")},
        {QStringLiteral("常规"), QStringLiteral("General")},
        {QStringLiteral("左摇杆（移动鼠标）"), QStringLiteral("Left stick (cursor)")},
        {QStringLiteral("X 灵敏度:"), QStringLiteral("X sensitivity:")},
        {QStringLiteral("Y 灵敏度:"), QStringLiteral("Y sensitivity:")},
        {QStringLiteral("死区:"), QStringLiteral("Dead zone:")},
        {QStringLiteral("启用鼠标加速"), QStringLiteral("Enable mouse acceleration")},
        {QStringLiteral("右摇杆（滚动）"), QStringLiteral("Right stick (scroll)")},
        {QStringLiteral("垂直滚动速度:"), QStringLiteral("Vertical scroll speed:")},
        {QStringLiteral("水平滚动速度:"), QStringLiteral("Horizontal scroll speed:")},
        {QStringLiteral("摇杆灵敏度"), QStringLiteral("Stick sensitivity")},
        {QStringLiteral("操作方案(Profile)"), QStringLiteral("Profiles")},
        {QStringLiteral("当前方案:"), QStringLiteral("Active profile:")},
        {QStringLiteral("新建"), QStringLiteral("New")},
        {QStringLiteral("重命名"), QStringLiteral("Rename")},
        {QStringLiteral("删除"), QStringLiteral("Delete")},
        {QStringLiteral("恢复默认"), QStringLiteral("Reset to defaults")},
        {QStringLiteral("L3 层（按住 L3）"), QStringLiteral("L3 layer (hold L3)")},
        {QStringLiteral("RT 层（按住 RT）"), QStringLiteral("RT layer (hold RT)")},
        {QStringLiteral("按键映射"), QStringLiteral("Button mappings")},
        {QStringLiteral("启用自动切换模式"), QStringLiteral("Enable auto mode switch")},
        {QStringLiteral("检测间隔:"), QStringLiteral("Poll interval:")},
        {QStringLiteral("规则：仅在<b>鼠标模式</b>下检测；检测到玩游戏时自动切到<b>默认模式</b>。"
                        "<b>默认模式</b>不会被自动改变，需手动切回鼠标模式。"),
         QStringLiteral("Rules: detection only runs in <b>Mouse mode</b>; when gaming is "
                        "detected it switches to <b>Default mode</b>. <b>Default mode</b> is "
                        "never changed automatically — switch back manually.")},
        {QStringLiteral("游戏进程检测"), QStringLiteral("Game process detection")},
        {QStringLiteral("启用（命中配置的游戏进程即视为在玩游戏）"),
         QStringLiteral("Enable (a configured process running counts as gaming)")},
        {QStringLiteral("游戏进程名（每行一个，例如 game.exe）:"),
         QStringLiteral("Game process names (one per line, e.g. game.exe):")},
        {QStringLiteral("全屏窗口检测"), QStringLiteral("Fullscreen detection")},
        {QStringLiteral("前台窗口全屏时视为在玩游戏"),
         QStringLiteral("Treat a fullscreen foreground window as gaming")},
        {QStringLiteral("CPU 占用检测"), QStringLiteral("CPU usage detection")},
        {QStringLiteral("启用"), QStringLiteral("Enable")},
        {QStringLiteral("CPU 占用阈值:"), QStringLiteral("CPU usage threshold:")},
        {QStringLiteral("持续时长（超过此时长才触发）:"),
         QStringLiteral("Sustain time (must exceed to trigger):")},
        {QStringLiteral("GPU 占用检测（近似值）"), QStringLiteral("GPU usage detection (approx.)")},
        {QStringLiteral("GPU 占用阈值:"), QStringLiteral("GPU usage threshold:")},
        {QStringLiteral("持续时长:"), QStringLiteral("Sustain time:")},
        {QStringLiteral("提示：GPU 占用为近似值，依赖驱动支持；不同硬件可能不可用。"),
         QStringLiteral("Note: GPU usage is approximate, depends on driver support and may be "
                        "unavailable on some hardware.")},
        {QStringLiteral("自动切换"), QStringLiteral("Auto switch")},
        {QStringLiteral("新建操作方案"), QStringLiteral("Create profile")},
        {QStringLiteral("方案名称:"), QStringLiteral("Name:")},
        {QStringLiteral("新方案"), QStringLiteral("New Profile")},
        {QStringLiteral("无法创建:名称为空或已存在。"),
         QStringLiteral("Cannot create: the name is empty or already exists.")},
        {QStringLiteral("重命名操作方案"), QStringLiteral("Rename profile")},
        {QStringLiteral("新名称:"), QStringLiteral("New name:")},
        {QStringLiteral("无法重命名:新名称为空或已存在。"),
         QStringLiteral("Cannot rename: the new name is empty or already exists.")},
        {QStringLiteral("删除操作方案"), QStringLiteral("Delete profile")},
        {QStringLiteral("删除方案 \"%1\"?"), QStringLiteral("Delete profile \"%1\"?")},
        {QStringLiteral("至少需要保留一套方案。"),
         QStringLiteral("At least one profile must remain.")},
        {QStringLiteral("将方案 \"%1\" 的全部映射恢复为默认?"),
         QStringLiteral("Reset all mappings of profile \"%1\" to defaults?")},
    };
    return table;
}

} // namespace

Language currentLanguage() {
    return g_language;
}

void setLanguage(Language language) {
    g_language = language;
}

Language resolveUiLanguage(const QString& configValue) {
    if (configValue == QLatin1String("zh"))
        return Language::Chinese;
    if (configValue == QLatin1String("en"))
        return Language::English;
    // "system"(或未知取值):按操作系统 UI 语言解析。
    QStringList uiLangs = QLocale::system().uiLanguages();
    const QString primary = uiLangs.isEmpty() ? QLocale::system().name() : uiLangs.first();
    return primary.startsWith(QLatin1String("zh")) ? Language::Chinese : Language::English;
}

QString translate(const QString& source, Language language) {
    if (language == Language::English) {
        const auto it = englishTable().constFind(source);
        if (it != englishTable().constEnd())
            return *it;
    }
    return source; // 中文原文 / fail-safe 回退
}

QString tr(const QString& source) {
    return translate(source, g_language);
}

int englishEntryCount() {
    return static_cast<int>(englishTable().size());
}

bool englishTableContains(const QString& source) {
    return englishTable().contains(source);
}

} // namespace Translations
