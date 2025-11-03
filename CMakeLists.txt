cmake_minimum_required(VERSION 3.20)
project(SynGT_CPP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLI "Build console application" ON)
option(BUILD_GUI "Build Qt GUI application" ON)
option(BUILD_TESTS "Build unit tests" ON)

message(STATUS "=== SynGT C++ Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build CLI: ${BUILD_CLI}")
message(STATUS "Build GUI: ${BUILD_GUI}")
message(STATUS "Build Tests: ${BUILD_TESTS}")

add_subdirectory(libsyngt)

if(BUILD_CLI)
    add_subdirectory(syngt_cli)
endif()

if(BUILD_GUI)
    add_subdirectory(syngt_gui)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

find_package(Doxygen)
if(DOXYGEN_FOUND)
    option(BUILD_DOCS "Build documentation" OFF)
    if(BUILD_DOCS)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    endif()
endif()