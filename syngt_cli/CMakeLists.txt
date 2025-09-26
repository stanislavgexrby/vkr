cmake_minimum_required(VERSION 3.20)
project(syngt_cli VERSION 1.0.0 LANGUAGES CXX)

set(CLI_SOURCES
    main.cpp
)

add_executable(syngt_cli ${CLI_SOURCES})

set_target_properties(syngt_cli PROPERTIES
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(syngt_cli
    PRIVATE
        syngt::syngt
)

target_compile_features(syngt_cli PRIVATE cxx_std_17)

target_compile_options(syngt_cli PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(MINGW)
    target_link_options(syngt_cli PRIVATE
        -Wl,--subsystem,console
    )
endif()

set_target_properties(syngt_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS syngt_cli
    RUNTIME DESTINATION bin
)