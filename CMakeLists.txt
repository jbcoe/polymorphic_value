cmake_minimum_required(VERSION 2.8)

project(polymorphic_value)

if(MSVC)
  add_compile_options("/EHsc")
else()
  add_compile_options("-std=gnu++14")
  set(CMAKE_CXX_FLAGS_SANITIZE "-fsanitize=address,undefined")
  set(CMAKE_EXE_LINKER_FLAGS_SANITIZE "-fsanitize=address,undefined")
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  add_compile_options("-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option")
endif()

include_directories(externals/catch/single_include)
add_executable(test_polymorphic_value test_polymorphic_value.cpp)

enable_testing()
add_test(
  NAME test_polymorphic_value
  COMMAND test_polymorphic_value
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

