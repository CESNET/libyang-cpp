#
# Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
#
# Written by Václav Kubernát <kubernat@cesnet.cz>
#

project(libyang-cpp LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

set(CMAKE_CXX_FLAGS_DEBUG "-Werror ${CMAKE_CXX_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Woverloaded-virtual -Wimplicit-fallthrough -Wsuggest-override ${CMAKE_CXX_FLAGS}")

find_package(Doxygen)
option(WITH_DOCS "Create and install internal documentation (needs Doxygen)" ${DOXYGEN_FOUND})

find_package(PkgConfig)
pkg_check_modules(LIBYANG REQUIRED libyang>=2.0.7 IMPORTED_TARGET libyang)

add_library(libyang-cpp
    src/Context.cpp
    )

target_link_libraries(libyang-cpp PRIVATE PkgConfig::LIBYANG)

if(BUILD_TESTING)
    find_package(trompeloeil 33 REQUIRED)
    find_package(doctest 2.3.1 REQUIRED)

    add_library(DoctestIntegration STATIC
        tests/doctest_integration.cpp
        tests/trompeloeil_doctest.hpp
        )
    target_include_directories(DoctestIntegration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    target_link_libraries(DoctestIntegration doctest::doctest trompeloeil libyang-cpp)
    target_compile_definitions(DoctestIntegration PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)

    function(libyang_cpp_test name)
        add_executable(test_${name}
            tests/${name}.cpp
            )
        target_link_libraries(test_${name} DoctestIntegration)
        target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

        add_test(test_${name} test_${name})
        # FIXME: enable this after this gets fixed
        # https://github.com/CESNET/libyang/issues/1611
        # set_tests_properties(test_${name} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
    endfunction()

    libyang_cpp_test(dummy)
    libyang_cpp_test(context)
endif()
