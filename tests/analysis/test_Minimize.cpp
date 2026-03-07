#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/analysis/Minimize.h>

using namespace syngt;

class MinimizeTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }

    std::unique_ptr<Grammar> grammar;
};

TEST_F(MinimizeTest, SingleTerminalDoesNotCrash) {
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a'.");

    EXPECT_NO_THROW(Minimize::minimize(grammar.get()));

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(MinimizeTest, AlternativeTerminals) {
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' ; 'b' ; 'c'.");

    EXPECT_NO_THROW(Minimize::minimize(grammar.get()));

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(MinimizeTest, Sequence) {
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c'.");

    EXPECT_NO_THROW(Minimize::minimize(grammar.get()));

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(MinimizeTest, AllNTsPreserved) {
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->setNTRule("A", "'x' ; 'y'.");
    grammar->setNTRule("B", "'p' , 'q'.");

    EXPECT_NO_THROW(Minimize::minimize(grammar.get()));

    EXPECT_TRUE(grammar->getNTItem("A")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("B")->hasRoot());
}

TEST_F(MinimizeTest, EmptyGrammarDoesNotCrash) {
    EXPECT_NO_THROW(Minimize::minimize(grammar.get()));
}

TEST_F(MinimizeTest, RegularizedGrammarCanBeMinimized) {
    // Regularize first, then minimize — matches the Pascal workflow
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , 'n' ; 'n'.");

    EXPECT_NO_THROW(grammar->regularize());

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    ASSERT_TRUE(e->hasRoot());

    EXPECT_NO_THROW(Minimize::minimize(grammar.get()));

    e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(e->hasRoot());
}
