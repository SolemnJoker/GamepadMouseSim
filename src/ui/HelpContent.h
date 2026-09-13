#pragma once

#include <QString>
#include <QVector>

class Config;

// 帮助屏内容数据层(design.md D5):从"配置 + 共享默认表"的合并视图
// 生成分区行模型。**不依赖 QWidget**——内容正确性可离屏单测,视觉由
// OsdOverlay 绘制。替代已退役的构建期 help.png(帮助实时化)。
struct HelpLine {
    QString button;
    QString action;
};

struct HelpSection {
    QString title;
    QVector<HelpLine> lines;
};

namespace HelpContent {

// 按 config 的合并视图(配置 ⊕ MappingDefaults)生成帮助分区:
// 直接映射 / L3 层(按住 L3)/ RT 层(按住 RT)/ 摇杆 / 模式与系统。
// 值为 None 的行不展示;"模式与系统"区从配置动态抓取切换键与
// ShowHelp/ShowKeyboard 绑定,保证内容始终与生效映射一致。
QVector<HelpSection> build(const Config& config);

} // namespace HelpContent
