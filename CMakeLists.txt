cmake_minimum_required(VERSION 3.16)
project(InMotoManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # Q_OBJECT
set(CMAKE_AUTOUIC ON)   # .ui files
set(CMAKE_AUTORCC ON)   # .qrc files

# ---- Qt 5.15 ----
# Svg and Xml are needed by QLed (from qled.pri: QT += svg xml)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg Xml)

# ---- ROS 2 ----
# Find ROS even when Qt Creator was not started from a sourced terminal.
# Set ROS_DISTRO to your distro (humble, jazzy, ...).
if(DEFINED ENV{ROS_DISTRO})
    set(ROS_DISTRO $ENV{ROS_DISTRO})
else()
    set(ROS_DISTRO "humble" CACHE STRING "ROS 2 distro")
endif()
list(APPEND CMAKE_PREFIX_PATH /opt/ros/${ROS_DISTRO})
# Your colcon workspace, if you use custom interfaces:
# list(APPEND CMAKE_PREFIX_PATH $ENV{HOME}/ros2_ws/install/my_interfaces)

find_package(rclcpp REQUIRED)
find_package(std_srvs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(inmoto_ros REQUIRED)


# find_package(my_interfaces REQUIRED)

# ---- QLed (replaces include(/usr/local/src/QLed/qled.pri)) ----
set(QLED_DIR /usr/local/src/QLed)
set(QLED_SOURCES
    ${QLED_DIR}/qled.cpp
    ${QLED_DIR}/qled.h
    ${QLED_DIR}/qled.qrc
)

# ---- Application ----
add_executable(InMotoManager
    main.cpp
    inmotomanager.cpp
    inmotomanager.h
    inmotomanager.ui
    ${QLED_SOURCES}
)

target_include_directories(InMotoManager PRIVATE ${QLED_DIR})

target_compile_definitions(InMotoManager PRIVATE QT_DEPRECATED_WARNINGS)

target_link_libraries(InMotoManager PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Svg
    Qt5::Xml
    rclcpp::rclcpp
    ${std_srvs_TARGETS}
    ${std_msgs_TARGETS}
    ${inmoto_ros_TARGETS}
    # ${my_interfaces_TARGETS}
)

# Same as target.path in the .pro
install(TARGETS InMotoManager DESTINATION /opt/InMotoManager/bin)
