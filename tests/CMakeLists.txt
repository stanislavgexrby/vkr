cmake_minimum_required(VERSION 3.20)
project(syngt_tests VERSION 1.0.0 LANGUAGES CXX)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

set(TEST_SOURCES
    core/test_Grammar.cpp
    core/test_TerminalList.cpp
    core/test_NonTerminalList.cpp
    regex/test_RETree.cpp
    regex/test_RETerminal.cpp
    regex/test_REOr.cpp
    regex/test_REAnd.cpp
    parser/test_Parser.cpp
    parser/test_CharProducer.cpp
    transform/test_Transformation.cpp
    utils/test_UndoRedo.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_link_libraries(${TEST_NAME}
        PRIVATE
            syngt::syngt
            GTest::gtest
            GTest::gtest_main
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
endforeach()

add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)