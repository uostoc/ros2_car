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
find_package(sensor_msgs REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(car_operator_console
  include/car_operator_console/language_manager.hpp
  include/car_operator_console/main_window.hpp
  include/car_operator_console/ros_bridge.hpp
  src/language_manager.cpp
  src/main.cpp
  src/main_window.cpp
  src/ros_bridge.cpp
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
  sensor_msgs
)
target_link_libraries(car_operator_console Qt6::Widgets)

set(translation_ts_files
  translations/car_operator_console_en.ts
  translations/car_operator_console_zh_CN.ts
)
set_source_files_properties(${translation_ts_files}
  PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/translations")
qt_add_lrelease(car_operator_console
  TS_FILES ${translation_ts_files}
  QM_FILES_OUTPUT_VARIABLE translation_qm_files)
qt_add_resources(car_operator_console "car_operator_console_translation_resources"
  PREFIX "/i18n"
  BASE "${CMAKE_CURRENT_BINARY_DIR}/translations"
  FILES ${translation_qm_files})

install(TARGETS car_operator_console
  DESTINATION lib/${PROJECT_NAME})
install(FILES ${translation_qm_files}
  DESTINATION share/${PROJECT_NAME}/translations)

if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  add_executable(test_console_state test/test_console_state.cpp)
  target_compile_features(test_console_state PUBLIC cxx_std_17)
  target_include_directories(test_console_state PRIVATE include)
  target_link_libraries(test_console_state Qt6::Test)
  add_test(NAME test_console_state COMMAND test_console_state)

  add_executable(test_language_manager
    include/car_operator_console/language_manager.hpp
    include/car_operator_console/main_window.hpp
    include/car_operator_console/ros_bridge.hpp
    test/test_language_manager.cpp
    src/language_manager.cpp
    src/main_window.cpp
    src/ros_bridge.cpp
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
  )
  target_link_libraries(test_language_manager Qt6::Test Qt6::Widgets)
  add_dependencies(test_language_manager car_operator_console_lrelease)
  add_test(NAME test_language_manager COMMAND test_language_manager)
endif()

ament_package()
