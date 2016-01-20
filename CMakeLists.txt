cmake_minimum_required(VERSION 2.8)

project(cloning_ptr)

set(CMAKE_CXX_FLAGS "-std=c++14")

include_directories(externals/catch/include)
add_executable(TestCloningPtr TestCloningPtr.cpp)

enable_testing()
add_test(
  NAME TestCloningPtr
  COMMAND TestCloningPtr
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

