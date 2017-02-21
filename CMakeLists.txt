cmake_minimum_required(VERSION 2.8)

project(polymorphic_value)

if(MSVC)
  add_compile_options("/EHsc")
else()
  add_compile_options("-std=c++14")
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  add_compile_options("-Werror;-Wall")
endif()

include_directories(externals/catch/include)
add_executable(test_polymorphic_value test_polymorphic_value.cpp)

enable_testing()
add_test(
  NAME test_polymorphic_value
  COMMAND test_polymorphic_value
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

