cmake_minimum_required(VERSION 2.8)

project(deep_ptr)

set(CMAKE_CXX_FLAGS "-std=c++14")

include_directories(externals/catch/include)
add_executable(TestDeepPtr TestDeepPtr.cpp)

enable_testing()
add_test(
  NAME TestDeepPtr
  COMMAND TestDeepPtr
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

