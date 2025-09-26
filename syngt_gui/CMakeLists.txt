cmake_minimum_required(VERSION 3.20)
project(syngt_gui VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(GUI_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ChildWindow.cpp
    src/DiagramWidget.cpp
    src/AddDialog.cpp
    src/ChangeSizeDialog.cpp
    src/PrintDialog.cpp
    src/AnalyzerWindow.cpp
)

set(GUI_HEADERS
    include/MainWindow.h
    include/ChildWindow.h
    include/DiagramWidget.h
    include/AddDialog.h
    include/ChangeSizeDialog.h
    include/PrintDialog.h
    include/AnalyzerWindow.h
)

set(GUI_UI_FILES
    resources/ui/MainWindow.ui
    resources/ui/ChildWindow.ui
    resources/ui/AddDialog.ui
)

set(GUI_RESOURCES
    resources/resources.qrc
)

add_executable(syngt_gui
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_UI_FILES}
    ${GUI_RESOURCES}
)

target_include_directories(syngt_gui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(syngt_gui
    PRIVATE
        syngt::syngt
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
)

set_target_properties(syngt_gui PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS syngt_gui
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)