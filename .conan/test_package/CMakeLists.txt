project(test_package LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(polymorphic_value REQUIRED)
find_package(Catch2 REQUIRED)

add_executable(test_polymorphic_value ../../test_polymorphic_value.cpp)
target_link_libraries(test_polymorphic_value PRIVATE polymorphic_value::polymorphic_value Catch2::Catch2)

enable_testing()
include(Catch)
catch_discover_tests(test_polymorphic_value)