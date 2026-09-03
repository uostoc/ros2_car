cmake_minimum_required(VERSION 3.16)
project(car_operator_console)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(car_control_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(car_operator_console
  include/car_operator_console/main_window.hpp
  include/car_operator_console/ros_bridge.hpp
  src/main.cpp
  src/main_window.cpp
  src/ros_bridge.cpp
)
target_compile_features(car_operator_console PUBLIC cxx_std_17)
target_include_directories(car_operator_console PRIVATE include)
ament_target_dependencies(car_operator_console
  car_control_interfaces
  geometry_msgs
  nav2_msgs
  nav_msgs
  rclcpp
  rclcpp_action
  sensor_msgs
)
target_link_libraries(car_operator_console Qt6::Widgets)

install(TARGETS car_operator_console
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  add_executable(test_console_state test/test_console_state.cpp)
  target_compile_features(test_console_state PUBLIC cxx_std_17)
  target_include_directories(test_console_state PRIVATE include)
  target_link_libraries(test_console_state Qt6::Test)
  add_test(NAME test_console_state COMMAND test_console_state)
endif()

ament_package()
