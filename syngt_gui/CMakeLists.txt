cmake_minimum_required(VERSION 3.20)
project(syngt_gui VERSION 1.0.0 LANGUAGES CXX)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

set(GUI_SOURCES
    main.cpp
)

add_executable(syngt_gui WIN32 ${GUI_SOURCES} ${IMGUI_SOURCES})

target_include_directories(syngt_gui PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

target_link_libraries(syngt_gui
    PRIVATE
        syngt::syngt
        d3d11
        dxgi
)

set_target_properties(syngt_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)