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

# Real QSystemTrayIcon inspection test. Requires the SVG resources to be
# compiled in (so :/icons/mouse.svg resolves). Run under offscreen QPA.
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
add_test(NAME test_tray_icon         COMMAND test_tray_icon)
set_tests_properties(test_tray_icon PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
target_link_libraries(test_gamepad_poller PRIVATE
    Qt6::Test
    Qt6::Core
    src_core
    xinput
)
add_test(NAME test_gamepad_poller COMMAND test_gamepad_poller)
set_tests_properties(test_gamepad_poller PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\;C:/Qt/6.8.3/msvc2022_64/bin")