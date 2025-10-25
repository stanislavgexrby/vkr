cmake_minimum_required(VERSION 3.20)
project(libsyngt VERSION 1.0.0 LANGUAGES CXX)

set(LIBSYNGT_SOURCES
    # Core
    src/core/Grammar.cpp
    src/core/TerminalList.cpp
    src/core/NonTerminalList.cpp
    src/core/SemanticList.cpp
    src/core/MacroList.cpp
    src/core/NTListItem.cpp
    
    # Regex
    src/regex/RELeaf.cpp
    src/regex/RETerminal.cpp
    src/regex/RESemantic.cpp
    src/regex/RENonTerminal.cpp
    src/regex/REBinaryOp.cpp
    
    # Graphics
    src/graphics/Arrow.cpp
    src/graphics/DrawPoint.cpp
    src/graphics/DrawObject.cpp
    
    # Parser
    src/parser/Parser.cpp
    
    # Transform
    src/transform/LeftElimination.cpp
    src/transform/LeftFactorization.cpp
    src/transform/RemoveUseless.cpp
    src/transform/FirstFollow.cpp
    
    # Analysis
    src/analysis/ParsingTable.cpp
    
    # Utils
    src/utils/Semantic.cpp
    src/utils/UndoRedo.cpp
    src/utils/Creator.cpp
)

add_library(syngt STATIC ${LIBSYNGT_SOURCES})

add_library(syngt::syngt ALIAS syngt)

target_include_directories(syngt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(syngt PUBLIC cxx_std_17)

target_compile_options(syngt PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

set_target_properties(syngt PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "syngt"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

install(TARGETS syngt
    EXPORT syngt-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

install(EXPORT syngt-targets
    FILE syngt-targets.cmake
    NAMESPACE syngt::
    DESTINATION lib/cmake/syngt
)