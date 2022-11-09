#
# Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
#
# Written by Václav Kubernát <kubernat@cesnet.cz>
#

project(libyang-cpp LANGUAGES CXX)
cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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
prepare_git_version(LIBYANG_CPP_VERSION "0.0")

find_package(Doxygen)
option(WITH_DOCS "Create and install internal documentation (needs Doxygen)" ${DOXYGEN_FOUND})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBYANG REQUIRED libyang>=2.1.4 IMPORTED_TARGET)
set(LIBYANG_CPP_PKG_VERSION "1.1.0")

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

add_library(yang-cpp SHARED
    src/ChildInstantiables.cpp
    src/Context.cpp
    src/DataNode.cpp
    src/Enum.cpp
    src/Collection.cpp
    src/Module.cpp
    src/SchemaNode.cpp
    src/Set.cpp
    src/Type.cpp
    src/Utils.cpp
    src/utils/exception.cpp
    src/utils/ref_count.cpp
    src/utils/newPath.cpp
    )

target_link_libraries(yang-cpp PRIVATE PkgConfig::LIBYANG)

include(GenerateExportHeader)
generate_export_header(yang-cpp BASE_NAME libyang_cpp EXPORT_FILE_NAME libyang-cpp/export.h)

if(BUILD_TESTING)
    find_package(doctest 2.3.6 REQUIRED)

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

# this is not enough, but at least it will generate the `install` target so that the CI setup is less magic
install(TARGETS yang-cpp)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libyang-cpp" TYPE INCLUDE)
install(DIRECTORY ${PROJECT_BINARY_DIR}/libyang-cpp TYPE INCLUDE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libyang-cpp.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
