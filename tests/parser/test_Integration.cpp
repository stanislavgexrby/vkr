#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <syngt/utils/Creator.h>
#include <syngt/graphics/DrawObject.h>

using namespace syngt;
using namespace syngt::graphics;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(IntegrationTest, ParseTransformVisualize) {
    grammar->addNonTerminal("E");
    grammar->addTerminal("+");
    grammar->addTerminal("n");
    
    // E -> E + n ; n
    grammar->setNTRule("E", "E , '+' , 'n' ; 'n'.");
    
    LeftElimination::eliminate(grammar.get());
    
    auto item = grammar->getNTItem("E");
    ASSERT_TRUE(item != nullptr);
    ASSERT_TRUE(item->hasRoot());
    
    auto list = std::make_unique<DrawObjectList>(grammar.get());
    Creator::createDrawObjects(list.get(), item->root());
    
    EXPECT_GT(list->count(), 0);
    EXPECT_GT(list->width(), 0);
}

TEST_F(IntegrationTest, LoadFactorizeSave) {
    grammar->addNonTerminal("S");
    grammar->addTerminal("a");
    grammar->addTerminal("b");
    grammar->addTerminal("c");
    
    // S -> a b ; a c
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");

    auto item = grammar->getNTItem("S");
    LeftFactorization::factorize(item, grammar.get());

    ASSERT_TRUE(item != nullptr);
    ASSERT_TRUE(item->hasRoot());
}

TEST_F(IntegrationTest, MultipleTransformations) {
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    grammar->addTerminal("x");
    grammar->addTerminal("y");
    
    grammar->setNTRule("A", "B , 'x' ; C.");
    grammar->setNTRule("B", "'y'.");
    grammar->setNTRule("C", "'@'.");
    
    RemoveUseless::remove(grammar.get());
    
    EXPECT_TRUE(grammar->getNTItem("A")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("B")->hasRoot());
}

TEST_F(IntegrationTest, MultipleNonTerminals) {
    grammar->addNonTerminal("program");
    grammar->addNonTerminal("statement");
    grammar->addNonTerminal("expression");
    grammar->addTerminal("begin");
    grammar->addTerminal("end");
    grammar->addTerminal("id");
    
    grammar->setNTRule("program", "'begin' , statement , 'end'.");
    grammar->setNTRule("statement", "expression.");
    grammar->setNTRule("expression", "'id'.");
    
    EXPECT_TRUE(grammar->getNTItem("program")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("statement")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("expression")->hasRoot());
    
    auto list = std::make_unique<DrawObjectList>(grammar.get());
    
    for (const auto& ntName : grammar->getNonTerminals()) {
        auto item = grammar->getNTItem(ntName);
        if (item && item->hasRoot()) {
            Creator::createDrawObjects(list.get(), item->root());
            EXPECT_GT(list->count(), 0);
        }
    }
}

TEST_F(IntegrationTest, LargeGrammar) {
    for (int i = 0; i < 20; i++) {
        grammar->addNonTerminal("NT" + std::to_string(i));
    }
    
    for (int i = 0; i < 30; i++) {
        grammar->addTerminal("term" + std::to_string(i));
    }
    
    for (int i = 0; i < 20; i++) {
        std::string rule = "'term" + std::to_string(i) + "'.";
        grammar->setNTRule("NT" + std::to_string(i), rule);
    }
    
    EXPECT_EQ(grammar->getNonTerminals().size(), 21);
    EXPECT_GE(grammar->getTerminals().size(), 30);
}

TEST_F(IntegrationTest, GrammarCloning) {
    grammar->addTerminal("token");
    grammar->addNonTerminal("rule");
    grammar->setNTRule("rule", "'token'.");
    
    auto item = grammar->getNTItem("rule");
    ASSERT_TRUE(item != nullptr);
    
    auto copiedTree = item->copyRETree();
    EXPECT_TRUE(copiedTree != nullptr);
    
    EXPECT_EQ(item->root()->toString(SelectionMask(), false),
              copiedTree->toString(SelectionMask(), false));
}