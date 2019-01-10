cmake_minimum_required(VERSION 3.1)

project(polymorphic_value)

option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)

add_subdirectory(externals/catch)

add_library(polymorphic_value INTERFACE)
target_sources(polymorphic_value
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_value.h"
)

add_executable(test_polymorphic_value test_polymorphic_value.cpp)
target_link_libraries(test_polymorphic_value
    PRIVATE
        polymorphic_value
        Catch2::Catch2
)

target_compile_options(test_polymorphic_value
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option>
)

set_target_properties(test_polymorphic_value PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if (ENABLE_SANITIZERS)
    set(SANITIZER_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")
    set(SANITIZER_FLAGS_UBSAN "-fsanitize=undefined")

    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("${SANITIZER_FLAGS_ASAN}" COMPILER_SUPPORTS_ASAN)
    check_cxx_compiler_flag("${SANITIZER_FLAGS_UBSAN}" COMPILER_SUPPORTS_UBSAN)

    if (COMPILER_SUPPORTS_ASAN)
        add_library(asan INTERFACE IMPORTED)
        set_target_properties(asan PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_ASAN}"
            INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_ASAN}"
        )
        target_link_libraries(test_polymorphic_value
            PRIVATE
                 asan
        )
    endif(COMPILER_SUPPORTS_ASAN)

    if (COMPILER_SUPPORTS_UBSAN)
        add_library(ubsan INTERFACE IMPORTED)
        set_target_properties(ubsan PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
            INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
        )
        target_link_libraries(test_polymorphic_value
            PRIVATE
                 ubsan
        )
    endif(COMPILER_SUPPORTS_UBSAN)
endif(ENABLE_SANITIZERS)

enable_testing()
add_test(
    NAME test_polymorphic_value
    COMMAND test_polymorphic_value
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/externals/catch/contrib)
include(Catch)
catch_discover_tests(test_polymorphic_value)

