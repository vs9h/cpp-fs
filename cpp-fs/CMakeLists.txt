cmake_minimum_required(VERSION 3.27)
project(cpp-fs)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

include(cmake/BuildOptions.cmake)
include(cmake/PreventInSourceBuilds.cmake)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

add_subdirectory(storage)
add_subdirectory(balancer)
