cmake_minimum_required(VERSION 2.8)

project(poly)

set(CMAKE_CXX_FLAGS "-std=c++14")

include_directories(externals/catch/include)
add_executable(TestPoly TestPoly.cpp)

enable_testing()
add_test(
  NAME TestPoly
  COMMAND TestPoly
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

