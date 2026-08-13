# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.16)
project(r200_validation LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(realsense2 REQUIRED CONFIG)

set(RS2_PROBES
  rs2_options
  rs2_pointcloud
  rs2_probe
  rs2_profile_matrix
  rs2_rate
  rs2_restart
  rs2_stream
  rs2_y16)

foreach(probe IN LISTS RS2_PROBES)
  add_executable(${probe} ${probe}.cpp)
  target_link_libraries(${probe} PRIVATE realsense2::realsense2)
endforeach()

add_executable(xu_test xu_test.c)

option(R200_BUILD_RS1_PROBES "Build probes against a librealsense 1.x installation" OFF)
if(R200_BUILD_RS1_PROBES)
  find_path(RS1_INCLUDE_DIR librealsense/rs.hpp REQUIRED)
  find_library(RS1_LIBRARY NAMES realsense REQUIRED)
  foreach(probe rs1_baseline rs1_controls rs1_probe)
    add_executable(${probe} ${probe}.cpp)
    target_include_directories(${probe} PRIVATE ${RS1_INCLUDE_DIR})
    target_link_libraries(${probe} PRIVATE ${RS1_LIBRARY})
  endforeach()
endif()

install(TARGETS ${RS2_PROBES} xu_test RUNTIME DESTINATION bin)
