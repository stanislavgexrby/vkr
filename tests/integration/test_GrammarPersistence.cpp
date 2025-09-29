#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <fstream>

using namespace syngt;

TEST(GrammarPersistenceTest, SaveAndLoad) {
    Grammar grammar1;
    grammar1.fillNew();
    
    grammar1.addNonTerminal("expr");
    
    grammar1.setNTRule("S", "'begin' , 'end'.");
    grammar1.setNTRule("expr", "term , ('+' ; '-') , term.");
    
    std::cout << "Before save:\n";
    std::cout << "  S has rule: " << grammar1.hasRule("S") << "\n";
    std::cout << "  expr has rule: " << grammar1.hasRule("expr") << "\n";
    
    std::string filename = "test_grammar.grm";
    grammar1.save(filename);
    
    std::ifstream check(filename);
    std::string line;
    std::cout << "\nSaved file contents:\n";
    while (std::getline(check, line)) {
        std::cout << "  " << line << "\n";
    }
    check.close();
    
    Grammar grammar2;
    grammar2.load(filename);
    
    std::cout << "\nAfter load:\n";
    std::cout << "  S has rule: " << grammar2.hasRule("S") << "\n";
    std::cout << "  expr has rule: " << grammar2.hasRule("expr") << "\n";
    
    EXPECT_TRUE(grammar2.hasRule("S"));
    EXPECT_TRUE(grammar2.hasRule("expr"));
    
    std::remove(filename.c_str());
}