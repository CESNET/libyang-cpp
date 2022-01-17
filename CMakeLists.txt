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

set(CMAKE_CXX_FLAGS_DEBUG "-Werror ${CMAKE_CXX_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Woverloaded-virtual -Wimplicit-fallthrough -Wsuggest-override ${CMAKE_CXX_FLAGS}")

add_custom_target(libyang-cpp-version-cmake
        cmake/ProjectGitVersion.cmake
        cmake/ProjectGitVersionRunner.cmake
        )
include(cmake/ProjectGitVersion.cmake)
prepare_git_version(LIBYANG_CPP_VERSION "0.0")

find_package(Doxygen)
option(WITH_DOCS "Create and install internal documentation (needs Doxygen)" ${DOXYGEN_FOUND})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBYANG REQUIRED libyang>=2.0.7 IMPORTED_TARGET libyang)
set(LIBYANG_CPP_PKG_VERSION "1.0.0")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(yang-cpp SHARED
    src/ChildInstantiables.cpp
    src/Context.cpp
    src/DataNode.cpp
    src/Enum.cpp
    src/Collection.cpp
    src/Module.cpp
    src/SchemaNode.cpp
    src/Set.cpp
    src/String.cpp
    src/Type.cpp
    src/Utils.cpp
    src/utils/exception.cpp
    src/utils/ref_count.cpp
    src/utils/newPath.cpp
    )

target_link_libraries(yang-cpp PRIVATE PkgConfig::LIBYANG)

if(BUILD_TESTING)
    find_package(doctest 2.4.6 REQUIRED)

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
    libyang_cpp_test(misc)
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
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libyang-cpp.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
