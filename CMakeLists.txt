cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED PROJECT_NAME)
  set(POLYMORPHIC_IS_NOT_SUBPROJECT ON)
endif()

set(POLYMOPHIC_VALUE_VERSION 1.3.0) # Revision 3, see paper p0201r3

project(polymorphic_value LANGUAGES CXX  VERSION ${POLYMOPHIC_VALUE_VERSION})

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
endif()

option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)
option(ENABLE_CODE_COVERAGE "Enable code coverage if available (Mac OS X currently not supported)" OFF)

include(CTest)
include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(polymorphic_value INTERFACE)
target_include_directories(polymorphic_value
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(polymorphic_value
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_value.h>
        # Only include natvis files in Visual Studio
        $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_value.natvis>>
        $<INSTALL_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/polymorphic_value.natvis>>
)

target_compile_features(polymorphic_value
    INTERFACE
        cxx_std_17
)

# namespaced alias for users depending on polymorphic_value as a sub-project.
add_library(polymorphic_value::polymorphic_value ALIAS polymorphic_value)

if(POLYMORPHIC_IS_NOT_SUBPROJECT)

    if (${BUILD_TESTING})
        FetchContent_Declare(
            catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        )

        FetchContent_GetProperties(catch2)
        if(NOT catch2_POPULATED)
            FetchContent_Populate(catch2)
            add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        endif()

        add_executable(test_polymorphic_value test_polymorphic_value.cpp)
        target_link_libraries(test_polymorphic_value
            PRIVATE
                polymorphic_value::polymorphic_value
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

        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
        include(Catch)
        catch_discover_tests(test_polymorphic_value)
    endif(${BUILD_TESTING})

    if (APPLE)
        SET(ENABLE_CODE_COVERAGE OFF CACHE BOOL "Ensure code coverage is switched off for Mac OS until the code coverage library addresses the AppleClang issue" FORCE)
    endif()

    if (ENABLE_CODE_COVERAGE)
        include(FetchContent)
        FetchContent_Declare(
            codecoverage
            GIT_REPOSITORY https://github.com/RWTH-HPC/CMake-codecov.git
        )

        FetchContent_GetProperties(codecoverage)
        if(NOT codecoverage_POPULATED)
            FetchContent_Populate(codecoverage)
            list(APPEND CMAKE_MODULE_PATH ${codecoverage_SOURCE_DIR}/cmake)
        endif()

        set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage build." FORCE)
        find_package(codecov)
        add_coverage(test_polymorphic_value)
        list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")
        coverage_evaluate()
    endif()

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_value.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_value.natvis"
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        DESTINATION
            "${CMAKE_INSTALL_DATAROOTDIR}/licenses/polymorphic_value"
    )

    install(
        TARGETS polymorphic_value
        EXPORT polymorphic_value-target
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        EXPORT polymorphic_value-target
        NAMESPACE polymorphic_value::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/polymorphic_value"
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/polymorphic_value-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/polymorphic_value-config.cmake
        INSTALL_DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/polymorphic_value"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/polymorphic_value-version.cmake
        VERSION ${POLYMOPHIC_VALUE_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/polymorphic_value-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/polymorphic_value-version.cmake
        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/polymorphic_value"
    )
endif(POLYMORPHIC_IS_NOT_SUBPROJECT)

