cmake_minimum_required(VERSION 3.0)

project(polymorphic_value)

if(MSVC)
  add_compile_options("/EHsc")
else()
  add_compile_options("-std=gnu++14")
  set(CMAKE_CXX_FLAGS_SANITIZE "-fsanitize=address,undefined")
  set(CMAKE_EXE_LINKER_FLAGS_SANITIZE "-fsanitize=address,undefined")
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  add_compile_options("-g;-O0;-Werror;-Wall")
endif()

# Boost tests
add_executable(test_polymorphic_value test/test_polymorphic_value.cpp)

find_package(Boost REQUIRED)
target_include_directories(test_polymorphic_value PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_polymorphic_value PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(test_polymorphic_value ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

enable_testing()

add_test(
  NAME test_polymorphic_value
  COMMAND test_polymorphic_value
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

