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