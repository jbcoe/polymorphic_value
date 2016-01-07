cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)

project(deep_ptr)

include_directories(externals/catch/include)
add_executable(TestDeepPtr TestDeepPtr.cpp)

enable_testing()
add_test(
  NAME TestDeepPtr
  COMMAND TestDeepPtr
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

