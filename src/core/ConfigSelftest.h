#pragma once

// 进程级配置链自检(design.md D7 L4):以真实部署路径形态跑完整配置
// 链(qrc 资源 → 加载链 → 迁移 → profile 展开 → 合并视图),每项打印
// [PASS]/[FAIL],返回全部通过与否。由 `GamepadMouseSim.exe --selftest`
// 触发,ctest 直接以退出码断言。检查项与单元测试共享同一实现,不复制
// 逻辑——本模式新增的价值是"进程级 + 真实文件系统 IO"。
bool runConfigSelftest();
