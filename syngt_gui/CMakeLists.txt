cmake_minimum_required(VERSION 3.20)
project(syngt_gui VERSION 1.0.0 LANGUAGES CXX)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

if(NOT EXISTS ${IMGUI_DIR}/imgui.h)
    message(FATAL_ERROR "ImGui not found! Please run setup_imgui.bat first.")
endif()

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

if(WIN32)
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )
endif()

set(GUI_SOURCES
    main.cpp
)

add_executable(syngt_gui WIN32 ${GUI_SOURCES} ${IMGUI_SOURCES})

target_include_directories(syngt_gui 
    PRIVATE 
        ${IMGUI_DIR} 
        ${IMGUI_DIR}/backends
)

target_link_libraries(syngt_gui
    PRIVATE
        syngt::syngt
)

if(WIN32)
    target_link_libraries(syngt_gui
        PRIVATE
            d3d11
            dxgi
            d3dcompiler
            dwmapi
    )
endif()

target_compile_features(syngt_gui PRIVATE cxx_std_17)

target_compile_options(syngt_gui PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

set_target_properties(syngt_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS syngt_gui
    RUNTIME DESTINATION bin
)