cmake_minimum_required(VERSION 3.27)
project(cpp-fs)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

include(cmake/BuildOptions.cmake)
include(cmake/PreventInSourceBuilds.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)
find_package(tl-expected REQUIRED CONFIG)

enable_testing()

add_subdirectory(storage)
