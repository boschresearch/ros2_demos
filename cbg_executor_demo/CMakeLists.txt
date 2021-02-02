cmake_minimum_required(VERSION 3.5)
project(cbg_executor_demo)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rcl REQUIRED)
find_package(rclc REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

include_directories(include)

add_executable(ping_node src/main.cpp src/cbg_executor_demo/ping_node.cpp src/cbg_executor_demo/parameter_helper.cpp)
ament_target_dependencies(ping_node rclcpp std_msgs)

add_executable(pong_rclc_node src/cbg_executor_demo/pong_rclc_node.c)
ament_target_dependencies(pong_rclc_node rcl rclc std_msgs)

install(TARGETS ping_node pong_rclc_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_dependencies(rclcpp std_msgs)
ament_package()
