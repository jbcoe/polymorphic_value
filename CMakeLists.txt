cmake_minimum_required(VERSION 2.8)

project(cloned_ptr)

set(CMAKE_CXX_FLAGS "-std=c++14")

include_directories(externals/catch/include)
add_executable(TestClonedPtr TestClonedPtr.cpp)

enable_testing()
add_test(
  NAME TestClonedPtr
  COMMAND TestClonedPtr
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

