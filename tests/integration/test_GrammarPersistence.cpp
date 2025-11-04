#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <fstream>

using namespace syngt;

TEST(GrammarPersistenceTest, SaveAndLoad) {
    Grammar grammar1;
    grammar1.fillNew();
    
    grammar1.addNonTerminal("start");
    grammar1.addNonTerminal("expr");
    
    grammar1.setNTRule("start", "'begin' , 'end'.");
    grammar1.setNTRule("expr", "term , ('+' ; '-') , term.");
    
    std::string filename = "test_grammar.grm";
    grammar1.save(filename);
    
    Grammar grammar2;
    grammar2.load(filename);
    
    EXPECT_TRUE(grammar2.hasRule("start"));
    EXPECT_TRUE(grammar2.hasRule("expr"));
    
    std::remove(filename.c_str());
}