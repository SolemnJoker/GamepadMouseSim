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

add_test(NAME test_config_autostart COMMAND test_config_autostart)
add_test(NAME test_autostart_apply    COMMAND test_autostart_apply)