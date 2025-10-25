#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>

using namespace syngt;

TEST(GrammarTest, Initialization) {
    Grammar grammar;
    grammar.fillNew();
    
    EXPECT_EQ(grammar.getNonTerminals().size(), 1);
    EXPECT_EQ(grammar.getNonTerminals()[0], "S");
}

TEST(GrammarTest, AddTerminals) {
    Grammar grammar;
    
    int id1 = grammar.addTerminal("begin");
    int id2 = grammar.addTerminal("end");
    int id3 = grammar.addTerminal("begin");  // Duplicate
    
    EXPECT_EQ(id1, 0);
    EXPECT_EQ(id2, 1);
    EXPECT_EQ(id3, 0);
    EXPECT_EQ(grammar.getTerminals().size(), 2);
}

TEST(GrammarTest, FindTerminals) {
    Grammar grammar;
    
    grammar.addTerminal("begin");
    grammar.addTerminal("end");
    
    EXPECT_EQ(grammar.findTerminal("begin"), 0);
    EXPECT_EQ(grammar.findTerminal("end"), 1);
    EXPECT_EQ(grammar.findTerminal("middle"), -1);
}

TEST(GrammarTest, AddMultipleTypes) {
    Grammar grammar;
    grammar.fillNew();
    
    grammar.addTerminal("program");
    grammar.addTerminal("begin");
    grammar.addTerminal("end");
    
    grammar.addNonTerminal("statement");
    grammar.addNonTerminal("expression");
    
    grammar.addSemantic("@action1");
    
    EXPECT_GE(grammar.getTerminals().size(), 3);
}