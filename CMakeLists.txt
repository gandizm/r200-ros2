cmake_minimum_required(VERSION 3.8)
project(r200_demo)

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME})
install(PROGRAMS scripts/r200_switch.sh
  DESTINATION lib/${PROJECT_NAME})

ament_package()
