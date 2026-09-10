cmake_minimum_required(VERSION 3.16)
project(car_operator_console)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(car_control_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_default_plugins REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2_msgs REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets LinguistTools Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(car_operator_console
  include/car_operator_console/language_manager.hpp
  include/car_operator_console/main_window.hpp
  include/car_operator_console/ros_bridge.hpp
  include/car_operator_console/teleop_key_bindings.hpp
  src/language_manager.cpp
  src/main.cpp
  src/main_window.cpp
  src/ros_bridge.cpp
  src/teleop_key_bindings.cpp
)
target_compile_features(car_operator_console PUBLIC cxx_std_17)
target_include_directories(car_operator_console PRIVATE include)
ament_target_dependencies(car_operator_console
  ament_index_cpp
  car_control_interfaces
  geometry_msgs
  nav2_msgs
  nav_msgs
  rclcpp
  rclcpp_action
  rviz_common
  rviz_default_plugins
  sensor_msgs
  tf2_msgs
)
target_link_libraries(car_operator_console
  Qt5::Widgets
  rviz_common::rviz_common
  rviz_default_plugins::rviz_default_plugins
)

set(translation_ts_files
  translations/car_operator_console_en.ts
  translations/car_operator_console_zh_CN.ts
)
set_source_files_properties(${translation_ts_files}
  PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/translations")
qt5_add_translation(translation_qm_files ${translation_ts_files})
add_custom_target(car_operator_console_lrelease ALL DEPENDS ${translation_qm_files})
add_dependencies(car_operator_console car_operator_console_lrelease)

install(TARGETS car_operator_console
  DESTINATION lib/${PROJECT_NAME})
install(FILES ${translation_qm_files}
  DESTINATION share/${PROJECT_NAME}/translations)
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config)

if(BUILD_TESTING)
  add_executable(test_console_state test/test_console_state.cpp)
  target_compile_features(test_console_state PUBLIC cxx_std_17)
  target_include_directories(test_console_state PRIVATE include)
  target_link_libraries(test_console_state Qt5::Test)
  add_test(NAME test_console_state COMMAND test_console_state)

  add_executable(test_language_manager
    include/car_operator_console/language_manager.hpp
    include/car_operator_console/main_window.hpp
    include/car_operator_console/ros_bridge.hpp
    test/test_language_manager.cpp
    src/language_manager.cpp
    src/main_window.cpp
    src/ros_bridge.cpp
    src/teleop_key_bindings.cpp
  )
  target_compile_features(test_language_manager PUBLIC cxx_std_17)
  target_include_directories(test_language_manager PRIVATE include)
  target_compile_definitions(test_language_manager PRIVATE
    CAR_OPERATOR_CONSOLE_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}/translations")
  ament_target_dependencies(test_language_manager
    ament_index_cpp
    car_control_interfaces
    geometry_msgs
    nav2_msgs
    nav_msgs
    rclcpp
    rclcpp_action
    sensor_msgs
    tf2_msgs
  )
  target_link_libraries(test_language_manager
    Qt5::Test
    Qt5::Widgets
    rviz_common::rviz_common
    rviz_default_plugins::rviz_default_plugins)
  add_dependencies(test_language_manager car_operator_console_lrelease)
  add_test(NAME test_language_manager COMMAND test_language_manager)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_logs")
  set_tests_properties(test_language_manager PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ROS_LOG_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_logs")

  add_executable(test_teleop_key_bindings
    include/car_operator_console/teleop_key_bindings.hpp
    src/teleop_key_bindings.cpp
    test/test_teleop_key_bindings.cpp
  )
  target_compile_features(test_teleop_key_bindings PUBLIC cxx_std_17)
  target_include_directories(test_teleop_key_bindings PRIVATE include)
  target_link_libraries(test_teleop_key_bindings Qt5::Test)
  add_test(NAME test_teleop_key_bindings COMMAND test_teleop_key_bindings)
endif()

ament_package()
