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
    src/core/Types.cpp
    
    # Regex
    src/regex/RETree.cpp
    src/regex/RELeaf.cpp
    src/regex/RETerminal.cpp
    src/regex/RESemantic.cpp
    src/regex/REMacro.cpp
    src/regex/RENonTerminal.cpp
    src/regex/REBinaryOp.cpp
    src/regex/REOr.cpp
    src/regex/REAnd.cpp
    src/regex/REIteration.cpp
    
    # Graphics
    src/graphics/DrawObject.cpp
    src/graphics/DrawObjectList.cpp
    src/graphics/DrawPoint.cpp
    src/graphics/DrawObjectPoint.cpp
    src/graphics/DrawObjectExtended.cpp
    src/graphics/DrawObjectBorder.cpp
    src/graphics/DrawObjectTerminal.cpp
    src/graphics/DrawObjectNonTerminal.cpp
    src/graphics/DrawObjectMacro.cpp
    src/graphics/Arrow.cpp
    src/graphics/SemanticArrow.cpp
    src/graphics/Ward.cpp
    
    # Parser
    src/parser/Parser.cpp
    src/parser/Parser2.cpp
    src/parser/CharProducer.cpp
    src/parser/ErrorHandler.cpp
    
    # Transform
    src/transform/Transformation.cpp
    src/transform/TransGrammar.cpp
    src/transform/TransRE_Tree.cpp
    src/transform/TransRE_Terminal.cpp
    src/transform/TransRE_Semantic.cpp
    src/transform/TransRE_Nonterminal.cpp
    src/transform/TransRE_Or.cpp
    src/transform/TransRE_And.cpp
    src/transform/TransRE_Iteration.cpp
    src/transform/TransRE_BinaryOp.cpp
    src/transform/TransRE_Leaf.cpp
    src/transform/TransLeaf.cpp
    src/transform/TransNonterminal.cpp
    src/transform/TransTerminal.cpp
    src/transform/TransSemantic.cpp
    src/transform/TransParser.cpp
    src/transform/TransParser2.cpp
    src/transform/TransCharProducer.cpp
    src/transform/TransErrorUnit.cpp
    src/transform/TransCreator.cpp
    src/transform/TransTransformation.cpp
    src/transform/LeftElimination.cpp
    src/transform/LeftFactorization.cpp
    src/transform/RemoveUseless.cpp
    src/transform/FirstFollow.cpp
    
    # Analysis
    src/analysis/Analyzer.cpp
    src/analysis/ParsingTable.cpp
    src/analysis/Minimization.cpp
    src/analysis/DugaAutomat.cpp
    
    # Utils
    src/utils/UndoRedo.cpp
    src/utils/Creator.cpp
    src/utils/Dictionary.cpp
    src/utils/Semantic.cpp
    src/utils/FileIO.cpp
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