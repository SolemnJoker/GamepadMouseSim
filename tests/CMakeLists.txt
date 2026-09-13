find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(test_config_autostart
    test_main.cpp
    test_config_autostart.cpp
)
target_link_libraries(test_config_autostart PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)

qt_add_executable(test_autostart_apply
    test_autostart_main.cpp
    test_autostart_apply.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AutoStart.cpp
)
target_link_libraries(test_autostart_apply PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)

# Real QSystemTrayIcon inspection test binary. The binary WORKS on a
# real Windows shell but causes a stack overflow (0xc0000409) when run
# with QT_QPA_PLATFORM=offscreen (which ctest uses). We therefore build
# the binary but EXCLUDE it from `ctest`. See spec §7.4 row 1 PARTIAL
# carve-out for the manual-smoke fallback.
qt_add_executable(test_tray_icon
    test_tray_icon_main.cpp
    test_tray_icon.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/SystemTray.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_tray_icon PRIVATE
    Qt6::Widgets
    Qt6::Svg
    Qt6::Core
    Qt6::Test
    src_core
)

add_test(NAME test_config_autostart COMMAND test_config_autostart)
add_test(NAME test_autostart_apply    COMMAND test_autostart_apply)

# Log severity: captures qWarning/qCritical/qDebug output via a custom
# message handler and asserts the correct severity labels are applied.
qt_add_executable(test_log_severity
    test_log_severity_main.cpp
    test_log_severity.cpp
)
target_link_libraries(test_log_severity PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_log_severity COMMAND test_log_severity)

# actionToChinese: every ButtonAction enum value must have a non-empty label.
qt_add_executable(test_action_to_chinese
    test_action_to_chinese_main.cpp
    test_action_to_chinese.cpp
)
target_link_libraries(test_action_to_chinese PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_action_to_chinese COMMAND test_action_to_chinese)

# Gamepad input simulation: FakeXInput feeds scripted button sequences
# through ComboKeyDetector + ModeManager to verify LT+View mode switch.
qt_add_executable(test_gamepad_poller
    test_gamepad_poller_main.cpp
    test_gamepad_poller.cpp
    ${CMAKE_SOURCE_DIR}/src/gamepad/ComboKeyDetector.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ModeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/win/XInputWrapper.cpp
)
file(GLOB _win_kit_um_x64 "C:/Program Files (x86)/Windows Kits/10/Lib/*/um/x64")
find_library(XINPUT_LIBRARY NAMES xinput1_4 xinput HINTS ${_win_kit_um_x64} REQUIRED)
target_link_libraries(test_gamepad_poller PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
    ${XINPUT_LIBRARY}
)
add_test(NAME test_gamepad_poller COMMAND test_gamepad_poller)
set_tests_properties(test_gamepad_poller PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ---------------------------------------------------------------------------
# Virtual keyboard (openspec change: add-virtual-keyboard)
# ---------------------------------------------------------------------------
# Navigation: pure logic driven by synthetic stick/dpad quantization; the
# injection stream is observed through FakeKeyInjector (no real SendInput,
# no QWidget — logic/window separation per design.md D9).
qt_add_executable(test_keyboard_nav
    test_keyboard_nav.cpp
    ${CMAKE_SOURCE_DIR}/src/input/KeyboardNavController.cpp
)
target_link_libraries(test_keyboard_nav PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_keyboard_nav COMMAND test_keyboard_nav)
set_tests_properties(test_keyboard_nav PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Controller: toggle open/close detection, input bypass while open, mode
# constraints, multi-gamepad routing — against FakeOverlay + FakeKeyInjector.
qt_add_executable(test_keyboard_controller
    test_keyboard_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/input/KeyboardController.cpp
    ${CMAKE_SOURCE_DIR}/src/input/KeyboardNavController.cpp
)
target_link_libraries(test_keyboard_controller PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_keyboard_controller COMMAND test_keyboard_controller)
set_tests_properties(test_keyboard_controller PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Mapping defaults merge (design.md D8): old configs without the new keys
# still dispatch ShowKeyboard via the built-in fallback table.
qt_add_executable(test_keyboard_mapping
    test_keyboard_mapping.cpp
    ${CMAKE_SOURCE_DIR}/src/input/KeyboardMapper.cpp
    ${CMAKE_SOURCE_DIR}/src/win/SendInputHelper.cpp
)
target_link_libraries(test_keyboard_mapping PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_keyboard_mapping COMMAND test_keyboard_mapping)
set_tests_properties(test_keyboard_mapping PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
# MappingDefaults: single source of truth for default mappings (design D2),
# plus consistency check of the bundled default_config.json projection.
qt_add_executable(test_mapping_defaults
    test_mapping_defaults.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_mapping_defaults PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_mapping_defaults COMMAND test_mapping_defaults)
set_tests_properties(test_mapping_defaults PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Config load chain (design D1): file -> qrc builtin -> skeleton, with
# auto-writeback of a writable config.json, and incremental v1->v2 migration.
qt_add_executable(test_config_load_chain
    test_config_load_chain.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_config_load_chain PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_config_load_chain COMMAND test_config_load_chain)
set_tests_properties(test_config_load_chain PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Profiles: expansion/collect mirror semantics and management rules (design D3).
qt_add_executable(test_config_profiles
    test_config_profiles.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_config_profiles PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_config_profiles COMMAND test_config_profiles)
set_tests_properties(test_config_profiles PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ---------------------------------------------------------------------------
# add-config-profiles-and-live-help
# ---------------------------------------------------------------------------
# L4 process-level selftest: the app itself verifies the full config chain
# (qrc resource -> load chain -> migration -> profile expansion -> merged
# view) and exits 0/1. ctest asserts the exit code.
add_test(NAME selftest_config COMMAND $<TARGET_FILE:GamepadMouseSim> --selftest)
set_tests_properties(selftest_config PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Help content data layer (L1): content matches the merged view and follows
# config changes. No QWidget involved.
qt_add_executable(test_help_content
    test_help_content.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/HelpContent.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_help_content PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
)
add_test(NAME test_help_content COMMAND test_help_content)
set_tests_properties(test_help_content PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# L3 feasibility spike: plain QWidget + grab() under offscreen QPA.
qt_add_executable(test_offscreen_render
    test_offscreen_render.cpp
)
target_link_libraries(test_offscreen_render PRIVATE
    Qt6::Test
    Qt6::Widgets
)
# D7 L3 downgrade (2026-09-13 spike): QApplication under this non-interactive
# shell hangs on both offscreen and windows platform plugins (same root cause
# as the tray-icon stack overflow). Binaries stay buildable for manual runs on
# an interactive desktop; excluded from ctest per the tray-icon precedent.
# add_test intentionally NOT called for the three render tests below.

# L3 keyboard overlay: rendered grid matches widget size; highlight movement
# changes pixels.
qt_add_executable(test_keyboard_overlay_render
    test_keyboard_overlay_render.cpp
    ${CMAKE_SOURCE_DIR}/src/input/KeyboardNavController.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/KeyboardOverlay.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_keyboard_overlay_render PRIVATE
    Qt6::Test
    Qt6::Widgets
    src_core
)


# L3 help overlay: renders content and re-renders on config change.
qt_add_executable(test_help_overlay_render
    test_help_overlay_render.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/HelpContent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OsdOverlay.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(test_help_overlay_render PRIVATE
    Qt6::Test
    Qt6::Widgets
    src_core
)

