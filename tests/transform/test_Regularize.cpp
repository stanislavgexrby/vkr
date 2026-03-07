#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/transform/Regularize.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/RightElimination.h>

using namespace syngt;

class RegularizeTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }

    std::unique_ptr<Grammar> grammar;
};

// ---------------------------------------------------------------------------
// Smoke tests — must not crash
// ---------------------------------------------------------------------------

TEST_F(RegularizeTest, SimpleLeftRecursiveGrammar) {
    // E : E , '+' , T ; T.
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("T");
    grammar->setNTRule("E", "E , '+' , T ; T.");
    grammar->setNTRule("T", "'id'.");

    EXPECT_NO_THROW(Regularize::regularize(grammar.get()));

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(e->hasRoot());
    // After regularization no direct left recursion remains
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
}

TEST_F(RegularizeTest, SimpleRightRecursiveGrammar) {
    // A : 'a' , A ; 'b'.
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; 'b'.");

    EXPECT_NO_THROW(Regularize::regularize(grammar.get()));

    NTListItem* a = grammar->getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(a->hasRoot());
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, grammar.get()));
}

TEST_F(RegularizeTest, PlainGrammarUnchanged) {
    // S : 'a' , 'b'.  — no recursion
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b'.");

    EXPECT_NO_THROW(Regularize::regularize(grammar.get()));

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(RegularizeTest, ClassicExpressionGrammar) {
    // expr : expr , '+' , term ; term.
    // term : term , '*' , factor ; factor.
    // factor : '(' , expr , ')' ; 'id'.
    grammar->addNonTerminal("expr");
    grammar->addNonTerminal("term");
    grammar->addNonTerminal("factor");
    grammar->setNTRule("expr",   "expr , '+' , term ; term.");
    grammar->setNTRule("term",   "term , '*' , factor ; factor.");
    grammar->setNTRule("factor", "'(' , expr , ')' ; 'id'.");

    EXPECT_NO_THROW(Regularize::regularize(grammar.get()));

    // All NTs must still have valid rules
    for (const auto& name : grammar->getNonTerminals()) {
        NTListItem* nt = grammar->getNTItem(name);
        if (nt) {
            EXPECT_TRUE(nt->hasRoot()) << "NT '" << name << "' lost its rule";
        }
    }
}

TEST_F(RegularizeTest, EmptyGrammarDoesNotCrash) {
    EXPECT_NO_THROW(Regularize::regularize(grammar.get()));
}

TEST_F(RegularizeTest, MultipleNTs_AllPreserved) {
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    grammar->setNTRule("A", "A , 'x' ; 'y'.");
    grammar->setNTRule("B", "'p' , B ; 'q'.");
    grammar->setNTRule("C", "'c'.");

    EXPECT_NO_THROW(Regularize::regularize(grammar.get()));

    // After regularization, all NTs must still exist and have rules
    EXPECT_TRUE(grammar->getNTItem("A")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("B")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("C")->hasRoot());
}

// ---------------------------------------------------------------------------
// Regularize vs grammar->regularize() — both call the same logic
// ---------------------------------------------------------------------------

TEST_F(RegularizeTest, SameResultAsGrammarMethod) {
    // Build two identical grammars
    auto grammar2 = std::make_unique<Grammar>();
    grammar2->fillNew();

    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , 'n' ; 'n'.");

    grammar2->addNonTerminal("E");
    grammar2->setNTRule("E", "E , '+' , 'n' ; 'n'.");

    Regularize::regularize(grammar.get());
    grammar2->regularize();

    NTListItem* e1 = grammar->getNTItem("E");
    NTListItem* e2 = grammar2->getNTItem("E");
    ASSERT_NE(e1, nullptr);
    ASSERT_NE(e2, nullptr);

    // Both should produce the same string representation
    EXPECT_EQ(e1->value(), e2->value());
}
