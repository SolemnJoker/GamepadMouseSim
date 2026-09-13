#pragma once

#include <QString>

// 界面多语言支持(design.md D1):轻量运行时查表,**中文原文为 key**。
// - tr(zh) 在当前语言表中查找译文;缺失时 fail-safe 回退中文原文
//   (界面永不出现 key 名/空串)。
// - 语言经 ui.language 配置键选择("system"/"zh"/"en"),"system" 按
//   操作系统 locale 解析。
// - 新增语言:在本模块追加一张译文表并在合法取值/设置下拉中登记,
//   界面调用点零改动(spec: 新增语言扩展)。
namespace Translations {

enum class Language { Chinese, English };

Language currentLanguage();
void setLanguage(Language language);

// ui.language 取值 → 具体语言;"system" 按 QLocale 的 UI 语言解析
// (zh 系 → 中文,其余 → 英文)。未知取值按 "system" 处理。
Language resolveUiLanguage(const QString& configValue);

// 中文原文 → 当前语言。所有界面文案调用点统一经此函数
// (设计 D4:源码扫描测试断言每条 tr 字面量都在英文表内)。
QString tr(const QString& source);

// 指定语言翻译(测试/导出用);缺译文回退 source。
QString translate(const QString& source, Language language);

// 英文表条目数(测试用)。
int englishEntryCount();

// 英文表是否包含该 key(测试用;identity 条目如 "Tab" 不能用
// translate 结果与原文相等来判断存在性)。
bool englishTableContains(const QString& source);

} // namespace Translations
