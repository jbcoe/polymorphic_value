cmake_minimum_required(VERSION 2.8)

project(indirect)

set(CMAKE_CXX_FLAGS "-std=c++14")

include_directories(externals/catch/include)
add_executable(TestIndirect TestIndirect.cpp)

enable_testing()
add_test(
  NAME TestIndirect
  COMMAND TestIndirect
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

