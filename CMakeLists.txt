cmake_minimum_required(VERSION 3.16)

project(AgricultureSensorDashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 QUIET COMPONENTS SerialPort)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/SensorCard.cpp
    src/SensorData.cpp
    src/DataProvider.cpp
)

set(HEADERS
    src/MainWindow.h
    src/SensorCard.h
    src/SensorData.h
    src/DataProvider.h
)

qt_add_executable(AgricultureSensorDashboard
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
)

target_include_directories(AgricultureSensorDashboard PRIVATE src)

target_link_libraries(AgricultureSensorDashboard PRIVATE
    Qt6::Widgets
)

if(TARGET Qt6::SerialPort)
    target_link_libraries(AgricultureSensorDashboard PRIVATE Qt6::SerialPort)
    target_compile_definitions(AgricultureSensorDashboard PRIVATE HAS_QT_SERIALPORT)
    message(STATUS "Qt SerialPort found — RS485/ESP32 serial support enabled")
else()
    message(WARNING "Qt SerialPort not installed — building in simulation-only mode")
endif()

if(WIN32)
    set_target_properties(AgricultureSensorDashboard PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
