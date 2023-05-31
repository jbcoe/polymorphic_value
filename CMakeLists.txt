cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0127 NEW)

if(NOT DEFINED PROJECT_NAME)
  set(POLYMORPHIC_IS_NOT_SUBPROJECT ON)
endif()

set(POLYMORPHIC_VERSION 2.0.0)

project(polymorphic LANGUAGES CXX  VERSION ${POLYMORPHIC_VERSION})

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

add_subdirectory(documentation)

add_library(polymorphic INTERFACE)
target_include_directories(polymorphic
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(polymorphic
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
        # Only include natvis files in Visual Studio
        $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.natvis>>
        $<INSTALL_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/polymorphic.natvis>>
)

target_compile_features(polymorphic
    INTERFACE
        cxx_std_17
)

# namespaced alias for users depending on polymorphic as a sub-project.
add_library(polymorphic::polymorphic ALIAS polymorphic)

if(POLYMORPHIC_IS_NOT_SUBPROJECT)

    if (${BUILD_TESTING})
        FetchContent_Declare(
            catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.3.2
        )

        # TODO(Avoid gloablly setting C++ standard) Force C++17 for Catchv2
        # Fix for: https://stackoverflow.com/questions/66227246/catch2-undefined-reference-to-catchstringmaker
        set(CMAKE_CXX_STANDARD 17)

       FetchContent_GetProperties(catch2)
        if(NOT catch2_POPULATED)
            FetchContent_Populate(catch2)
            add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        endif()

        add_executable(polymorphic_test polymorphic_test.cpp)
        target_link_libraries(polymorphic_test
            PRIVATE
                polymorphic::polymorphic
                Catch2::Catch2WithMain
        )

        target_compile_options(polymorphic_test
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option>
        )

        set_target_properties(polymorphic_test PROPERTIES
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
                target_link_libraries(polymorphic_test
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
                target_link_libraries(polymorphic_test
                    PRIVATE
                         ubsan
                )
            endif(COMPILER_SUPPORTS_UBSAN)
        endif(ENABLE_SANITIZERS)

        enable_testing()
        add_test(
            NAME polymorphic_test
            COMMAND polymorphic_test
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        )

        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
        include(Catch)
        catch_discover_tests(polymorphic_test)
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
        add_coverage(polymorphic_test)
        list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")
        coverage_evaluate()
    endif()

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.natvis"
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        DESTINATION
            "${CMAKE_INSTALL_DATAROOTDIR}/licenses/polymorphic"
    )

    install(
        TARGETS polymorphic
        EXPORT polymorphic-target
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        EXPORT polymorphic-target
        NAMESPACE polymorphic::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/polymorphic"
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/polymorphic-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/polymorphic-config.cmake
        INSTALL_DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/polymorphic"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/polymorphic-version.cmake
        VERSION ${POLYMORPHIC_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/polymorphic-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/polymorphic-version.cmake
        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/polymorphic"
    )
endif(POLYMORPHIC_IS_NOT_SUBPROJECT)

