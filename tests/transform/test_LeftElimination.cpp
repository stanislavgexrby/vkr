#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/transform/LeftElimination.h>

using namespace syngt;

class LeftEliminationTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(LeftEliminationTest, DetectDirectLeftRecursion) {
    // E → E '+' term | term
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , term ; term.");
    
    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    
    bool hasRecursion = LeftElimination::hasDirectLeftRecursion(e);
    EXPECT_TRUE(hasRecursion);
}

TEST_F(LeftEliminationTest, NoLeftRecursion) {
    // E → term '+' E | term
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "term , '+' , E ; term.");
    
    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    
    bool hasRecursion = LeftElimination::hasDirectLeftRecursion(e);
    EXPECT_FALSE(hasRecursion);
}

TEST_F(LeftEliminationTest, EliminateSimpleLeftRecursion) {
    // E → E '+' term | term
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("term");
    grammar->setNTRule("E", "E , '+' , term ; term.");
    
    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(e));
    
    LeftElimination::eliminateForNonTerminal(e, grammar.get());
    
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    
    NTListItem* eRec = grammar->getNTItem("E_rec");
    EXPECT_NE(eRec, nullptr);
    
    EXPECT_TRUE(e->hasRoot());
    std::string eRule = e->value();
    EXPECT_NE(eRule.find("term"), std::string::npos);
    EXPECT_NE(eRule.find("E_rec"), std::string::npos);
}

TEST_F(LeftEliminationTest, EliminateMultipleAlternatives) {
    // E → E '+' | E '-' | num
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' ; E , '-' ; num.");
    
    LeftElimination::eliminateForNonTerminal(grammar->getNTItem("E"), grammar.get());
    
    NTListItem* e = grammar->getNTItem("E");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    
    EXPECT_NE(grammar->getNTItem("E_rec"), nullptr);
}

TEST_F(LeftEliminationTest, EliminateForWholeGrammar) {
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("T");
    grammar->setNTRule("E", "E , '+' , T ; T.");
    grammar->setNTRule("T", "T , '*' , F ; F.");
    
    LeftElimination::eliminate(grammar.get());
    
    NTListItem* e = grammar->getNTItem("E");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    EXPECT_NE(grammar->getNTItem("E_rec"), nullptr);
    
    NTListItem* t = grammar->getNTItem("T");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(t));
    EXPECT_NE(grammar->getNTItem("T_rec"), nullptr);
}

TEST_F(LeftEliminationTest, SaveAndLoadAfterElimination) {
    grammar->addNonTerminal("expr");
    grammar->setNTRule("expr", "expr , '+' , term ; term.");
    
    LeftElimination::eliminate(grammar.get());
    
    std::string filename = "test_left_elim.grm";
    grammar->save(filename);
    
    Grammar grammar2;
    grammar2.load(filename);
    
    NTListItem* expr = grammar2.getNTItem("expr");
    ASSERT_NE(expr, nullptr);
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(expr));
    
    EXPECT_NE(grammar2.getNTItem("expr_rec"), nullptr);
    
    std::remove(filename.c_str());
}