#include <iostream>
#include <string>
#include <syngt/core/Grammar.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <syngt/transform/FirstFollow.h>
#include <syngt/analysis/ParsingTable.h>

using namespace syngt;

void printUsage(const char* progName) {
    std::cout << "SynGT C++ - Syntax Grammar Transformation Tool\n";
    std::cout << "Version 1.0 (Pascal port)\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  info <grammar.grm>                    - Show grammar information\n";
    std::cout << "  regularize <in.grm> <out.grm>         - Apply all transformations\n";
    std::cout << "  eliminate-left <in.grm> <out.grm>     - Eliminate left recursion\n";
    std::cout << "  factorize <in.grm> <out.grm>          - Apply left factorization\n";
    std::cout << "  remove-useless <in.grm> <out.grm>     - Remove useless symbols\n";
    std::cout << "  check-ll1 <grammar.grm>               - Check if grammar is LL(1)\n";
    std::cout << "  first-follow <grammar.grm>            - Compute and print FIRST/FOLLOW\n";
    std::cout << "  table <grammar.grm>                   - Generate parsing table\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << progName << " info examples/LANG.GRM\n";
    std::cout << "  " << progName << " regularize input.grm output.grm\n";
    std::cout << "  " << progName << " check-ll1 grammar.grm\n";
}

int cmdInfo(const std::string& filename) {
    try {
        Grammar grammar;
        grammar.load(filename);
        
        std::cout << "\n=== Grammar Information ===\n";
        std::cout << "File: " << filename << "\n\n";
        
        std::cout << "Terminals (" << grammar.terminals()->getCount() << "):\n  ";
        auto terms = grammar.terminals()->getItems();
        for (size_t i = 0; i < terms.size(); ++i) {
            std::cout << terms[i];
            if (i < terms.size() - 1) std::cout << ", ";
        }
        std::cout << "\n\n";
        
        std::cout << "Non-terminals (" << grammar.getNonTerminals().size() << "):\n";
        auto nts = grammar.getNonTerminals();
        int withRules = 0;
        for (const auto& nt : nts) {
            NTListItem* item = grammar.getNTItem(nt);
            if (item && item->hasRoot()) {
                std::cout << "  " << nt << " → " << item->value() << "\n";
                withRules++;
            } else {
                std::cout << "  " << nt << " → (no rule)\n";
            }
        }
        
        std::cout << "\nNon-terminals with rules: " << withRules << "/" << nts.size() << "\n";
        
        if (grammar.semantics()->getCount() > 0) {
            std::cout << "\nSemantics (" << grammar.semantics()->getCount() << "):\n  ";
            auto sems = grammar.semantics()->getItems();
            for (size_t i = 0; i < sems.size(); ++i) {
                std::cout << sems[i];
                if (i < sems.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdRegularize(const std::string& input, const std::string& output) {
    try {
        Grammar grammar;
        std::cout << "Loading grammar from: " << input << "\n";
        grammar.load(input);
        
        size_t beforeNTs = grammar.getNonTerminals().size();
        
        std::cout << "Applying transformations...\n";
        std::cout << "  1. Eliminating left recursion...\n";
        LeftElimination::eliminate(&grammar);
        
        std::cout << "  2. Left factorization...\n";
        LeftFactorization::factorizeAll(&grammar);
        
        std::cout << "  3. Removing useless symbols...\n";
        RemoveUseless::remove(&grammar);
        
        size_t afterNTs = grammar.getNonTerminals().size();
        
        std::cout << "Saving to: " << output << "\n";
        grammar.save(output);
        
        std::cout << "\nDone!\n";
        std::cout << "Non-terminals: " << beforeNTs << " → " << afterNTs << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdEliminateLeft(const std::string& input, const std::string& output) {
    try {
        Grammar grammar;
        grammar.load(input);
        
        std::cout << "Eliminating left recursion...\n";
        LeftElimination::eliminate(&grammar);
        
        grammar.save(output);
        std::cout << "Done! Saved to: " << output << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdFactorize(const std::string& input, const std::string& output) {
    try {
        Grammar grammar;
        grammar.load(input);
        
        std::cout << "Applying left factorization...\n";
        LeftFactorization::factorizeAll(&grammar);
        
        grammar.save(output);
        std::cout << "Done! Saved to: " << output << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdRemoveUseless(const std::string& input, const std::string& output) {
    try {
        Grammar grammar;
        grammar.load(input);
        
        std::cout << "Removing useless symbols...\n";
        RemoveUseless::remove(&grammar);
        
        grammar.save(output);
        std::cout << "Done! Saved to: " << output << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdCheckLL1(const std::string& filename) {
    try {
        Grammar grammar;
        grammar.load(filename);
        
        std::cout << "Checking if grammar is LL(1)...\n";
        
        bool isLL1 = FirstFollow::isLL1(&grammar);
        
        if (isLL1) {
            std::cout << "\nGrammar is LL(1)\n";
            return 0;
        } else {
            std::cout << "\nGrammar is NOT LL(1)\n";
            std::cout << "\nSuggestion: Try applying transformations:\n";
            std::cout << "  " << "syngt_cli regularize " << filename << " output.grm\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdFirstFollow(const std::string& filename) {
    try {
        Grammar grammar;
        grammar.load(filename);
        
        auto firstSets = FirstFollow::computeFirst(&grammar);
        auto followSets = FirstFollow::computeFollow(&grammar, firstSets);
        
        FirstFollow::printSets(&grammar, firstSets, followSets);
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmdTable(const std::string& filename) {
    try {
        Grammar grammar;
        grammar.load(filename);
        
        auto table = ParsingTable::build(&grammar);
        if (!table) {
            std::cerr << "Failed to build parsing table\n";
            return 1;
        }
        
        table->print(&grammar);
        
        if (table->hasConflicts()) {
            std::cout << "\nWarning: Grammar has conflicts (not LL(1))\n";
            std::cout << "Try: syngt_cli regularize " << filename << " output.grm\n";
            return 1;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: info <grammar.grm>\n";
            return 1;
        }
        return cmdInfo(argv[2]);
    }
    else if (command == "regularize") {
        if (argc < 4) {
            std::cerr << "Usage: regularize <input.grm> <output.grm>\n";
            return 1;
        }
        return cmdRegularize(argv[2], argv[3]);
    }
    else if (command == "eliminate-left") {
        if (argc < 4) {
            std::cerr << "Usage: eliminate-left <input.grm> <output.grm>\n";
            return 1;
        }
        return cmdEliminateLeft(argv[2], argv[3]);
    }
    else if (command == "factorize") {
        if (argc < 4) {
            std::cerr << "Usage: factorize <input.grm> <output.grm>\n";
            return 1;
        }
        return cmdFactorize(argv[2], argv[3]);
    }
    else if (command == "remove-useless") {
        if (argc < 4) {
            std::cerr << "Usage: remove-useless <input.grm> <output.grm>\n";
            return 1;
        }
        return cmdRemoveUseless(argv[2], argv[3]);
    }
    else if (command == "check-ll1") {
        if (argc < 3) {
            std::cerr << "Usage: check-ll1 <grammar.grm>\n";
            return 1;
        }
        return cmdCheckLL1(argv[2]);
    }
    else if (command == "first-follow") {
        if (argc < 3) {
            std::cerr << "Usage: first-follow <grammar.grm>\n";
            return 1;
        }
        return cmdFirstFollow(argv[2]);
    }
    else if (command == "table") {
        if (argc < 3) {
            std::cerr << "Usage: table <grammar.grm>\n";
            return 1;
        }
        return cmdTable(argv[2]);
    }
    else if (command == "--help" || command == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    else {
        std::cerr << "Unknown command: " << command << "\n\n";
        printUsage(argv[0]);
        return 1;
    }
}