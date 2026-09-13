# 静态编译 Qt 6.11.1 并构建单文件 EXE 指南

## 目标

将 GamepadMouseSim 编译为单个 `.exe` 文件，无需任何 Qt DLL 依赖。

## 环境信息

- **OS**: Windows 11 64位
- **Qt 版本**: 6.11.1
- **现有 Qt 安装**: `C:\Qt\6.11.1\mingw_64`（动态链接版，仅含 .dll 导入库）
- **MinGW**: `C:\Qt\Tools\mingw1310_64`（GCC 13.1.0）
- **CMake**: `C:\Qt\Tools\CMake_64\bin\cmake.exe`
- **磁盘空间**: C盘 312 GB 可用
- **项目路径**: `D:\project\sbgj`
- **构建路径**: `D:\project\sbgj\build`

## 第一步：下载 Qt 6.11.1 源码

```powershell
# 下载源码压缩包（约 800MB）
curl -L -o C:\Qt\6.11.1\qt-everywhere-src-6.11.1.tar.xz `
  "https://download.qt.io/official_releases/qt/6.11/6.11.1/single/qt-everywhere-src-6.11.1.tar.xz"
```

如果 curl 下载慢，也可以用浏览器或镜像站下载：
- 官方: https://download.qt.io/official_releases/qt/6.11/6.11.1/single/
- 清华镜像: https://mirrors.tuna.tsinghua.edu.cn/qt/official_releases/qt/6.11/6.11.1/single/

## 第二步：解压源码

```powershell
# 需要先安装 7-Zip（如果没有）
# 下载: https://www.7-zip.org/download.html

# 解压 .tar.xz（两步：先 xz 再 tar）
cd C:\Qt\6.11.1
& "C:\Program Files\7-Zip\7z.exe" x qt-everywhere-src-6.11.1.tar.xz
& "C:\Program Files\7-Zip\7z.exe" x qt-everywhere-src-6.11.1.tar

# 重命名为 Src
Rename-Item qt-everywhere-src-6.11.1 Src
```

解压后源码应在 `C:\Qt\6.11.1\Src\`

## 第三步：配置静态编译

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;$env:PATH"

# 创建构建目录
mkdir C:\Qt\6.11.1\build-static

cd C:\Qt\6.11.1\build-static

# 配置（跳过不需要的模块，大幅减少编译时间）
..\Src\configure.bat `
  -static `
  -release `
  -prefix C:\Qt\6.11.1\mingw_64_static `
  -platform win32-g++ `
  -skip qt3d -skip qt5compat -skip qtactiveqt -skip qtcharts `
  -skip qtconnectivity -skip qtdatavis3d -skip qtgraphs -skip qtgrpc `
  -skip qthttpserver -skip qtimageformats -skip qtlanguageserver `
  -skip qtlocation -skip qtlottie -skip qtmultimedia -skip qtnetworkauth `
  -skip qtopcua -skip qtpdf -skip qtpositioning -skip qtquick `
  -skip qtquick3d -skip qtquickeffectmaker -skip qtquicktimeline `
  -skip qtremoteobjects -skip qtscxml -skip qtsensors -skip qtserialbus `
  -skip qtserialport -skip qtspeech -skip qtsvg -skip qttools `
  -skip qttranslations -skip qtvirtualkeyboard -skip qtwayland `
  -skip qtwebchannel -skip qtwebengine -skip qtwebsockets -skip qtwebview `
  -nomake examples `
  -nomake tests `
  -no-opengl `
  -no-vulkan
```

**关键参数说明**：
- `-static`: 生成静态库 (.a) 而非动态库 (.dll)
- `-release`: 仅编译 Release 版本
- `-prefix`: 安装目标路径
- `-skip`: 跳过不需要的模块（项目只用 Core + Gui + Widgets）
- `-no-opengl -no-vulkan`: 项目不需要 GPU 加速

## 第四步：编译和安装

```powershell
cd C:\Qt\6.11.1\build-static

# 编译（约 30-60 分钟，取决于 CPU 核心数）
cmake --build . --parallel

# 安装到 C:\Qt\6.11.1\mingw_64_static
cmake --install .
```

完成后检查：`C:\Qt\6.11.1\mingw_64_static\lib\` 应有 `libQt6Core.a`、`libQt6Gui.a`、`libQt6Widgets.a` 等静态库文件。

## 第五步：修改项目 CMakeLists.txt

修改 `D:\project\sbgj\CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20)
project(GamepadMouseSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 指向静态 Qt
set(Qt6_DIR "C:/Qt/6.11.1/mingw_64_static/lib/cmake/Qt6")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/core/Config.cpp
    src/core/Types.cpp
    src/core/ModeManager.cpp
    src/gamepad/GamepadPoller.cpp
    src/gamepad/ComboKeyDetector.cpp
    src/input/InputMapper.cpp
    src/input/MouseMapper.cpp
    src/input/KeyboardMapper.cpp
    src/ui/SystemTray.cpp
    src/ui/OsdOverlay.cpp
    src/win/XInputWrapper.cpp
    src/win/SendInputHelper.cpp
)

set(HEADERS
    src/app/Application.h
    src/core/Config.h
    src/core/ModeManager.h
    src/core/Types.h
    src/gamepad/GamepadPoller.h
    src/gamepad/ComboKeyDetector.h
    src/input/InputMapper.h
    src/input/MouseMapper.h
    src/input/KeyboardMapper.h
    src/ui/SystemTray.h
    src/ui/OsdOverlay.h
    src/win/XInputWrapper.h
    src/win/SendInputHelper.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    xinput1_4
    -static-libgcc
    -static-libstdc++
    -static
    -loleaut32 -limm32 -lopengl32 -lversion -lwinmm
    -lws2_32 -luuid -lnetapi32 -luserenv -ldwmapi
)

# 不设 WIN32_EXECUTABLE，保留控制台窗口避免 Smart App Control 拦截
```

**关键改动**：
1. `set(Qt6_DIR ...)` 指向静态 Qt 的 cmake 目录
2. 添加 `-static-libgcc -static-libstdc++ -static` 静态链接所有运行时
3. 添加 Windows 平台库（Qt 静态链接需要这些）
4. 删除 `WIN32_EXECUTABLE` 设置

## 第六步：重新编译项目

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;$env:PATH"

# 清空旧构建
cd D:\project\sbgj\build
Remove-Item * -Recurse -Force

# 重新配置和编译
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
```

## 第七步：验证

```powershell
# 检查 exe 大小（静态链接后通常 15-30MB）
(Get-Item D:\project\sbgj\build\GamepadMouseSim.exe).Length / 1MB

# 在干净目录运行（不需要任何 Qt DLL）
mkdir D:\test-static
Copy-Item D:\project\sbgj\build\GamepadMouseSim.exe D:\test-static\
Copy-Item D:\project\sbgj\build\config D:\test-static\config -Recurse
cd D:\test-static
.\GamepadMouseSim.exe
```

如果正常运行，说明静态编译成功。

## 常见问题

**Q: configure 报错找不到 Perl/Python？**
A: 确保系统 PATH 中有 Perl（Strawberry Perl）和 Python。Qt configure 需要：
```powershell
winget install StrawberryPerl.Python.3.12
winget install StrawberryPerl.StrawberryPerl
```

**Q: 编译报错缺少某个 Windows 库？**
A: 在 `target_link_libraries` 中添加对应的 `-l<libname>`。常见缺失：
- `-ldwmapi` (DWM 相关)
- `-lshlwapi` (Shell 相关)
- `-lcrypt32` (加密相关)

**Q: 链接时报 undefined reference to `_Unwind_Resume`？**
A: 添加 `-lgcc_eh` 到链接列表。

**Q: Smart App Control 拦截 exe？**
A: 不要设置 `WIN32_EXECUTABLE TRUE`，保持控制台模式。或用户在 Windows 安全中心关闭 Smart App Control。
