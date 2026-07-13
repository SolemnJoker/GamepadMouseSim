cmake_minimum_required(VERSION 3.20)
project(GamepadMouseSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

# src_core: pure config + types static lib (no Win32/gamepad/input/ui deps).
# Lets future tests link Config without dragging in the rest of the engine.
add_library(src_core STATIC
    src/core/Config.cpp
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
    src/ui/SystemTray.cpp
    src/ui/OsdOverlay.cpp
    src/ui/SettingsDialog.cpp
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
    src/ui/SystemTray.h
    src/ui/OsdOverlay.h
    src/ui/SettingsDialog.h
    src/win/XInputWrapper.h
    src/win/SendInputHelper.h
)

set(RESOURCES
    resources/resources.qrc
)

# Pre-generate help screen image from Python script
set(HELP_PNG "${CMAKE_SOURCE_DIR}/resources/icons/help.png")
add_custom_command(
    OUTPUT "${HELP_PNG}"
    COMMAND python "${CMAKE_SOURCE_DIR}/tools/generate_help_png.py"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/generate_help_png.py"
    COMMENT "Generating help screen image"
)
add_custom_target(generate_help_png DEPENDS "${HELP_PNG}")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
add_dependencies(${PROJECT_NAME} generate_help_png)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    src_core
    Qt6::Widgets
    Qt6::Svg
    xinput
    oleaut32 imm32 opengl32 version winmm
    ws2_32 uuid netapi32 userenv dwmapi
    dxgi
)

# 不设 WIN32_EXECUTABLE，保留控制台窗口避免 Smart App Control 拦截


install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

add_subdirectory(tests)
