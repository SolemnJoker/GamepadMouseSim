cmake_minimum_required(VERSION 3.20)
project(GamepadMouseSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/core/Config.cpp
    src/core/Types.cpp
    src/core/ModeManager.cpp
    src/core/ProcessDetector.cpp
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
    src/core/ProcessDetector.h
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
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
