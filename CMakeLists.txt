cmake_minimum_required(VERSION 3.20)
project(GamepadMouseSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt is discovered via the standard CMake package mechanism — no hardcoded
# install path. Point the build at your Qt install with either
#   cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/<version>/<kit>"
# or the CMAKE_PREFIX_PATH environment variable.
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

# XInput import lib name differs between Windows SDK installs: some ship
# xinput1_4.lib, others only the xinput.lib alias, and find_library does not
# look inside Windows Kits Lib dirs by default — enumerate them as hints.
file(GLOB _win_kit_um_x64 "C:/Program Files (x86)/Windows Kits/10/Lib/*/um/x64")
find_library(XINPUT_LIBRARY NAMES xinput1_4 xinput HINTS ${_win_kit_um_x64} REQUIRED)
message(STATUS "XInput import library: ${XINPUT_LIBRARY}")

# src_core: pure config + types static lib (no Win32/gamepad/input/ui deps).
# Lets future tests link Config without dragging in the rest of the engine.
add_library(src_core STATIC
    src/core/Config.cpp
    src/core/ConfigSelftest.cpp
    src/core/MappingDefaults.cpp
    src/core/Types.cpp
)
target_include_directories(src_core PUBLIC src)
target_link_libraries(src_core PUBLIC Qt6::Core)

enable_testing()

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/core/ModeManager.cpp
    src/core/ProcessDetector.cpp
    src/core/AutoStart.cpp
    src/core/SysUsage.cpp
    src/core/GameDetector.cpp
    src/core/AutoModeController.cpp
    src/gamepad/GamepadPoller.cpp
    src/gamepad/ComboKeyDetector.cpp
    src/input/InputMapper.cpp
    src/input/MouseMapper.cpp
    src/input/KeyboardMapper.cpp
    src/input/KeyInjector.cpp
    src/input/KeyboardNavController.cpp
    src/input/KeyboardController.cpp
    src/ui/SystemTray.cpp
    src/ui/OsdOverlay.cpp
    src/ui/KeyboardOverlay.cpp
    src/ui/SettingsDialog.cpp
    src/ui/HelpContent.cpp
    src/win/XInputWrapper.cpp
    src/win/SendInputHelper.cpp
)

set(HEADERS
    src/app/Application.h
    src/core/ModeManager.h
    src/core/ProcessDetector.h
    src/core/AutoStart.h
    src/core/SysUsage.h
    src/core/GameDetector.h
    src/core/AutoModeController.h
    src/gamepad/GamepadPoller.h
    src/gamepad/ComboKeyDetector.h
    src/input/InputMapper.h
    src/input/MouseMapper.h
    src/input/KeyboardMapper.h
    src/input/KeyInjector.h
    src/input/KeyboardNavController.h
    src/input/KeyboardController.h
    src/ui/SystemTray.h
    src/ui/OsdOverlay.h
    src/ui/KeyboardOverlay.h
    src/ui/SettingsDialog.h
    src/ui/HelpContent.h
    src/win/XInputWrapper.h
    src/win/SendInputHelper.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    src_core
    Qt6::Widgets
    Qt6::Svg
    ${XINPUT_LIBRARY}
    oleaut32 imm32 opengl32 version winmm
    ws2_32 uuid netapi32 userenv dwmapi
    dxgi
)

# WIN32_EXECUTABLE: 无控制台窗口（GUI 应用）。调试输出仍写入 debug.log
# 和 OutputDebugString（见 src/main.cpp 的 qInstallMessageHandler）。
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)


install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

add_subdirectory(tests)
