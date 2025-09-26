cmake_minimum_required(VERSION 3.20)
project(syngt_examples VERSION 1.0.0 LANGUAGES CXX)

set(EXAMPLES
    usage/example_parse.cpp
    usage/example_transform.cpp
)

foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE})
    
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    target_link_libraries(${EXAMPLE_NAME}
        PRIVATE
            syngt::syngt
    )
    
    if(MINGW)
        target_link_options(${EXAMPLE_NAME} PRIVATE
            -Wl,--subsystem,console
        )
    endif()
    
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )
endforeach()