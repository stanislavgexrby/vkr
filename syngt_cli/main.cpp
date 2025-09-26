#include <iostream>
#include <syngt/core/Grammar.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "SynGT C++ - Syntax Grammar Transformation Tool\n";
        std::cout << "Usage: " << argv[0] << " <grammar_file.grm>\n";
        std::cout << "\nFor testing, run: ctest --output-on-failure\n";
        return 1;
    }
    
    try {
        syngt::Grammar grammar;
        grammar.load(argv[1]);
        
        std::cout << "Grammar loaded successfully!\n";
        std::cout << "  Terminals: " << grammar.getTerminals().size() << "\n";
        std::cout << "  NonTerminals: " << grammar.getNonTerminals().size() << "\n";
        std::cout << "  Semantics: " << grammar.getSemantics().size() << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}