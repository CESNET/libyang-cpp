#
# Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
#
# Written by Václav Kubernát <kubernat@cesnet.cz>
#

cmake_minimum_required(VERSION 3.22)
project(libyang-cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
include(CTest)
include(GNUInstallDirs)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-Werror ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Woverloaded-virtual -Wimplicit-fallthrough -Wsuggest-override ${CMAKE_CXX_FLAGS}")
endif()

add_custom_target(libyang-cpp-version-cmake
        cmake/ProjectGitVersion.cmake
        cmake/ProjectGitVersionRunner.cmake
        )
include(cmake/ProjectGitVersion.cmake)
set(LIBYANG_CPP_PKG_VERSION "4")
prepare_git_version(LIBYANG_CPP_VERSION ${LIBYANG_CPP_PKG_VERSION})

find_package(Doxygen)
option(WITH_DOCS "Create and install internal documentation (needs Doxygen)" ${DOXYGEN_FOUND})
option(BUILD_SHARED_LIBS "By default, shared libs are enabled. Turn off for a static build." ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBYANG REQUIRED libyang>=3.10.1 IMPORTED_TARGET)

# FIXME from gcc 14.1 on we should be able to use the calendar/time from libstdc++ and thus remove the date dependency
find_package(date)

# libyang::Regex::~Regex() bypasses libyang and calls out to libpcre directly
pkg_check_modules(LIBPCRE2 REQUIRED libpcre2-8 IMPORTED_TARGET)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include(CheckIncludeFileCXX)
check_include_file_cxx(experimental/iterator HAS_CXX_EXPERIMENTAL_ITERATOR)
if (NOT HAS_CXX_EXPERIMENTAL_ITERATOR)
    message(STATUS "C++ Library fundamentals TS2 <experimental/iterator> not found, using a backport")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/compat/experimental-iterator/)
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(yang-cpp
    src/ChildInstantiables.cpp
    src/Context.cpp
    src/DataNode.cpp
    src/Enum.cpp
    src/Collection.cpp
    src/Module.cpp
    src/Regex.cpp
    src/SchemaNode.cpp
    src/Set.cpp
    src/Type.cpp
    src/Utils.cpp
    src/utils/exception.cpp
    src/utils/ref_count.cpp
    src/utils/newPath.cpp
    )

target_link_libraries(yang-cpp PRIVATE PkgConfig::LIBYANG PkgConfig::LIBPCRE2)
# We do not offer any long-term API/ABI guarantees. To make stuff easier for downstream consumers,
# we will be bumping both API and ABI versions very deliberately.
# There will be no attempts at semver tracking, for example.
set_target_properties(yang-cpp PROPERTIES
    VERSION ${LIBYANG_CPP_PKG_VERSION}
    SOVERSION ${LIBYANG_CPP_PKG_VERSION})

include(GenerateExportHeader)
generate_export_header(yang-cpp BASE_NAME libyang_cpp EXPORT_FILE_NAME libyang-cpp/export.h)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    target_sources(yang-cpp INTERFACE FILE_SET HEADERS
        FILES
            libyang-cpp/Collection.hpp
            libyang-cpp/Context.hpp
            libyang-cpp/DataNode.hpp
            libyang-cpp/Enum.hpp
            libyang-cpp/ChildInstantiables.hpp
            libyang-cpp/Module.hpp
            libyang-cpp/Regex.hpp
            libyang-cpp/Set.hpp
            libyang-cpp/SchemaNode.hpp
            libyang-cpp/Time.hpp
            libyang-cpp/Type.hpp
            libyang-cpp/Utils.hpp
            libyang-cpp/Value.hpp
        )
    set_target_properties(yang-cpp PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
endif()

if(BUILD_TESTING)
    find_package(doctest 2.3.6 REQUIRED) # old version on Ubuntu 20.04 (that's used by TIP's OOPT build of this)

    add_library(DoctestIntegration STATIC
        tests/doctest-integration.cpp
        )
    target_include_directories(DoctestIntegration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/src/ "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(DoctestIntegration doctest::doctest)
    target_compile_definitions(DoctestIntegration PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tests/test_vars.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/test_vars.hpp" @ONLY)

    function(libyang_cpp_test name)
        add_executable(test_${name}
            tests/${name}.cpp
            )
        target_link_libraries(test_${name} DoctestIntegration yang-cpp)

        add_test(test_${name} test_${name})
    endfunction()

    libyang_cpp_test(context)
    libyang_cpp_test(data_node)
    target_link_libraries(test_data_node PkgConfig::LIBYANG)
    libyang_cpp_test(schema_node)
    libyang_cpp_test(unsafe)
    target_link_libraries(test_unsafe PkgConfig::LIBYANG)
    libyang_cpp_test(regex)

    if(date_FOUND)
        add_executable(test_time-stl-hhdate tests/time.cpp)
        target_link_libraries(test_time-stl-hhdate DoctestIntegration yang-cpp date::date-tz)
        add_test(test_time-stl-hhdate test_time-stl-hhdate)

        add_executable(test_time-hhdate tests/time.cpp)
        target_link_libraries(test_time-hhdate DoctestIntegration yang-cpp date::date-tz)
        target_compile_definitions(test_time-hhdate PUBLIC LIBYANG_CPP_SKIP_STD_CHRONO_TZ)
        add_test(test_time-hhdate test_time-hhdate)
    endif()
endif()

if(WITH_DOCS)
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)
    add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
            SOURCES ${doxyfile_in}
            )
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libyang-cpp.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/libyang-cpp.pc" @ONLY)

install(TARGETS yang-cpp)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libyang-cpp" TYPE INCLUDE)
install(DIRECTORY ${PROJECT_BINARY_DIR}/libyang-cpp TYPE INCLUDE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libyang-cpp.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
