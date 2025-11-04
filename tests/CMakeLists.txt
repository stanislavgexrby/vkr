cmake_minimum_required(VERSION 3.20)
project(syngt_tests VERSION 1.0.0 LANGUAGES CXX)

# Опции конфигурации тестов
option(TESTS_ENABLE_PARALLEL "Enable parallel test execution" ON)
option(TESTS_VERBOSE "Verbose test output" OFF)
set(TESTS_TIMEOUT "30" CACHE STRING "Test timeout in seconds")

# Find or download Google Test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "Google Test not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    message(STATUS "Google Test downloaded successfully")
endif()

enable_testing()

# Автоматически найти все test_*.cpp файлы
file(GLOB_RECURSE ALL_TEST_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*/test_*.cpp"
)

# Опциональное исключение определенных тестов
set(EXCLUDED_TESTS "" CACHE STRING "List of test files to exclude (semicolon-separated)")
if(EXCLUDED_TESTS)
    foreach(EXCLUDED ${EXCLUDED_TESTS})
        list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*${EXCLUDED}.*")
    endforeach()
    message(STATUS "Excluded tests: ${EXCLUDED_TESTS}")
endif()

# Создать словарь: категория -> список файлов
set(TEST_CATEGORIES "")
foreach(TEST_SOURCE ${ALL_TEST_SOURCES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_SOURCE})
    get_filename_component(TEST_DIR ${REL_PATH} DIRECTORY)
    
    if(NOT ${TEST_DIR} IN_LIST TEST_CATEGORIES)
        list(APPEND TEST_CATEGORIES ${TEST_DIR})
    endif()
endforeach()

list(SORT TEST_CATEGORIES)

message(STATUS "")
message(STATUS "=== Test Discovery ===")
message(STATUS "Scanning directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Found categories: ${TEST_CATEGORIES}")
message(STATUS "")

set(ALL_TEST_TARGETS "")
set(TOTAL_TESTS 0)

# Создать тесты по категориям
foreach(CATEGORY ${TEST_CATEGORIES})
    file(GLOB CATEGORY_TESTS 
        "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/test_*.cpp"
    )
    
    list(LENGTH CATEGORY_TESTS CATEGORY_COUNT)
    if(CATEGORY_COUNT GREATER 0)
        message(STATUS "[${CATEGORY}] Found ${CATEGORY_COUNT} test(s)")
        
        foreach(TEST_SOURCE ${CATEGORY_TESTS})
            get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
            
            # Убираем префикс test_ для читаемости
            string(REGEX REPLACE "^test_" "" TEST_BASE_NAME ${TEST_NAME})
            set(FULL_TEST_NAME "${TEST_NAME}")
            
            # Создать исполняемый файл
            add_executable(${FULL_TEST_NAME} ${TEST_SOURCE})
            
            target_link_libraries(${FULL_TEST_NAME}
                PRIVATE
                    syngt::syngt
                    GTest::gtest
                    GTest::gtest_main
            )
            
            target_compile_features(${FULL_TEST_NAME} PRIVATE cxx_std_17)
            
            # Опции компиляции
            target_compile_options(${FULL_TEST_NAME} PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
                $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
            )
            
            if(WIN32 AND MINGW)
                target_link_options(${FULL_TEST_NAME} PRIVATE -Wl,--subsystem,console)
            endif()
            
            # Добавить тест в CTest
            add_test(NAME ${FULL_TEST_NAME} COMMAND ${FULL_TEST_NAME})
            
            # Настройки теста
            set_tests_properties(${FULL_TEST_NAME} PROPERTIES
                LABELS "${CATEGORY}"
                TIMEOUT ${TESTS_TIMEOUT}
            )
            
            # Выходная директория
            set_target_properties(${FULL_TEST_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
            )
            
            list(APPEND ALL_TEST_TARGETS ${FULL_TEST_NAME})
            math(EXPR TOTAL_TESTS "${TOTAL_TESTS} + 1")
            
            # Детальный вывод
            file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_SOURCE})
            message(STATUS "  + ${REL_PATH} -> ${FULL_TEST_NAME}")
        endforeach()
        message(STATUS "")
    endif()
endforeach()

# Custom targets для запуска тестов по категориям
foreach(CATEGORY ${TEST_CATEGORIES})
    file(GLOB CATEGORY_TESTS 
        "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/test_*.cpp"
    )
    
    if(CATEGORY_TESTS)
        set(TARGET_NAME "test_category_${CATEGORY}")
        
        if(TESTS_ENABLE_PARALLEL)
            add_custom_target(${TARGET_NAME}
                COMMAND ${CMAKE_CTEST_COMMAND} -L ${CATEGORY} --output-on-failure -j ${CMAKE_BUILD_PARALLEL_LEVEL}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running ${CATEGORY} tests (parallel)..."
            )
        else()
            add_custom_target(${TARGET_NAME}
                COMMAND ${CMAKE_CTEST_COMMAND} -L ${CATEGORY} --output-on-failure
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running ${CATEGORY} tests..."
            )
        endif()
    endif()
endforeach()

# Custom target для запуска всех тестов
if(TESTS_ENABLE_PARALLEL)
    add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j ${CMAKE_BUILD_PARALLEL_LEVEL}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all tests (parallel)..."
    )
else()
    add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all tests..."
    )
endif()

# Target для быстрого запуска (только проваленные тесты)
add_custom_target(test_failed
    COMMAND ${CMAKE_CTEST_COMMAND} --rerun-failed --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Re-running failed tests..."
)

# Target для запуска с подробным выводом
add_custom_target(test_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests (verbose)..."
)

# Итоговая статистика
message(STATUS "=== Configuration Summary ===")
message(STATUS "Total test executables: ${TOTAL_TESTS}")
message(STATUS "Test categories: ${TEST_CATEGORIES}")
message(STATUS "Parallel execution: ${TESTS_ENABLE_PARALLEL}")
message(STATUS "Test timeout: ${TESTS_TIMEOUT} seconds")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/tests")
message(STATUS "")
message(STATUS "=== Available Commands ===")
message(STATUS "Run all tests:")
message(STATUS "  ctest --output-on-failure")
message(STATUS "  cmake --build . --target run_all_tests")
message(STATUS "")
message(STATUS "Run by category:")
foreach(CATEGORY ${TEST_CATEGORIES})
    file(GLOB CATEGORY_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/test_*.cpp")
    if(CATEGORY_TESTS)
        message(STATUS "  cmake --build . --target test_category_${CATEGORY}")
    endif()
endforeach()
message(STATUS "")
message(STATUS "Other targets:")
message(STATUS "  cmake --build . --target test_failed   # Re-run only failed tests")
message(STATUS "  cmake --build . --target test_verbose  # Verbose output")
message(STATUS "")
message(STATUS "Direct CTest:")
message(STATUS "  ctest -L <category>                    # Run specific category")
message(STATUS "  ctest -R <pattern>                     # Run tests matching pattern")
message(STATUS "  ctest -E <pattern>                     # Exclude tests matching pattern")
message(STATUS "  ctest --rerun-failed                   # Re-run failed tests")
message(STATUS "")