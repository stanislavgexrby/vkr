#include <iostream>
#include <syngt/core/Grammar.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "SynGT C++ - Syntax Grammar Transformation Tool\n\n";
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <input.grm>                    - Load and display info\n";
        std::cout << "  " << argv[0] << " <input.grm> <output.grm>       - Load, regularize, save\n";
        std::cout << "\nFor testing, run: ctest --output-on-failure\n";
        return 1;
    }
    
    try {
        syngt::Grammar grammar;
        
        std::cout << "Loading grammar from: " << argv[1] << "\n";
        grammar.load(argv[1]);
        
        std::cout << "Grammar loaded successfully!\n";
        std::cout << "  Terminals: " << grammar.getTerminals().size() << "\n";
        std::cout << "  NonTerminals: " << grammar.getNonTerminals().size() << "\n";
        std::cout << "  Semantics: " << grammar.getSemantics().size() << "\n";
        
        if (argc >= 3) {
            std::cout << "\nRegularizing grammar...\n";
            grammar.regularize();
            
            std::cout << "After regularization:\n";
            std::cout << "  NonTerminals: " << grammar.getNonTerminals().size() << "\n";
            
            std::cout << "\nSaving to: " << argv[2] << "\n";
            grammar.save(argv[2]);
            std::cout << "Done!\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}