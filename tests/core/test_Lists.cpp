#include <gtest/gtest.h>
#include <syngt/core/TerminalList.h>
#include <syngt/core/NonTerminalList.h>
#include <syngt/core/SemanticList.h>
#include <syngt/core/MacroList.h>

using namespace syngt;

TEST(TerminalListTest, AddAndFind) {
    TerminalList terminals;
    
    int id1 = terminals.add("program");
    int id2 = terminals.add("begin");
    int id3 = terminals.add("program");  // Duplicate
    
    EXPECT_EQ(id1, 0);
    EXPECT_EQ(id2, 1);
    EXPECT_EQ(id3, 0);  // Should return same ID
    EXPECT_EQ(terminals.getCount(), 2);
    EXPECT_EQ(terminals.find("begin"), 1);
    EXPECT_EQ(terminals.find("end"), -1);
}

TEST(NonTerminalListTest, FillNewAndAdd) {
    NonTerminalList nonTerminals;
    nonTerminals.fillNew();
    
    EXPECT_EQ(nonTerminals.getCount(), 1);
    EXPECT_EQ(nonTerminals.getString(0), "S");
    
    int id = nonTerminals.add("expression");
    EXPECT_EQ(id, 1);
    EXPECT_EQ(nonTerminals.getCount(), 2);
}

TEST(SemanticListTest, BasicOperations) {
    SemanticList semantics;
    
    int id1 = semantics.add("@action1");
    int id2 = semantics.add("@action2");
    
    EXPECT_EQ(id1, 0);
    EXPECT_EQ(id2, 1);
    EXPECT_EQ(semantics.getCount(), 2);
}

TEST(MacroListTest, BasicOperations) {
    MacroList macros;
    
    int id = macros.add("macro1");
    EXPECT_EQ(id, 0);
    EXPECT_EQ(macros.getCount(), 1);
}