#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/transform/RightElimination.h>

using namespace syngt;

class RightEliminationTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }

    std::unique_ptr<Grammar> grammar;
};

// ---------------------------------------------------------------------------
// Detection tests
// ---------------------------------------------------------------------------

TEST_F(RightEliminationTest, DetectDirectRightRecursion) {
    // A : 'a' , A ; 'b'.
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; 'b'.");

    NTListItem* a = grammar->getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(a, grammar.get()));
}

TEST_F(RightEliminationTest, NoRightRecursionInLeftRecursiveRule) {
    // E : E , '+' , term ; term.  — left-recursive, not right
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , term ; term.");

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(e, grammar.get()));
}

TEST_F(RightEliminationTest, NoRightRecursionInPlainRule) {
    // S : 'a' , 'b' , 'c'.
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c'.");

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(s, grammar.get()));
}

TEST_F(RightEliminationTest, DetectRightRecursionInOr) {
    // S : 'a' , S ; 'b'.  — right-recursive in first alternative
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , S ; 'b'.");

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(s, grammar.get()));
}

// ---------------------------------------------------------------------------
// Inline elimination tests — Pascal algorithm creates NO new NTs
// ---------------------------------------------------------------------------

TEST_F(RightEliminationTest, EliminateSimpleRightRecursion) {
    // A : 'a' , A ; 'b'.
    // By Arden: A = 'a' · A | 'b'  →  A = 'b' · 'a'*
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; 'b'.");

    NTListItem* a = grammar->getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(a, grammar.get()));

    size_t ntCountBefore = grammar->getNonTerminals().size();
    RightElimination::eliminateForNonTerminal(a, grammar.get());

    // No new NTs created (inline algorithm)
    EXPECT_EQ(grammar->getNonTerminals().size(), ntCountBefore);
    EXPECT_EQ(grammar->getNTItem("A_rec"), nullptr);

    // No right recursion after elimination
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, grammar.get()));

    // The rule must still exist and contain the base case
    EXPECT_TRUE(a->hasRoot());
    std::string rule = a->value();
    EXPECT_NE(rule.find("b"), std::string::npos);
}

TEST_F(RightEliminationTest, EliminateMultipleAlternatives) {
    // A : 'a' , A ; 'b' , A ; 'c'.
    // RA = 'a' | 'b', RB = 'c'  → A = 'c' · ('a' | 'b')*
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; 'b' , A ; 'c'.");

    NTListItem* a = grammar->getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(a, grammar.get()));

    size_t ntCountBefore = grammar->getNonTerminals().size();
    RightElimination::eliminateForNonTerminal(a, grammar.get());

    EXPECT_EQ(grammar->getNonTerminals().size(), ntCountBefore);
    EXPECT_EQ(grammar->getNTItem("A_rec"), nullptr);
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, grammar.get()));

    std::string rule = a->value();
    EXPECT_NE(rule.find("c"), std::string::npos);
}

TEST_F(RightEliminationTest, EliminateForWholeGrammar) {
    // A : 'a' , A ; 'b'.
    // B : 'x' , B ; 'y'.
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->setNTRule("A", "'a' , A ; 'b'.");
    grammar->setNTRule("B", "'x' , B ; 'y'.");

    size_t ntCountBefore = grammar->getNonTerminals().size();
    RightElimination::eliminate(grammar.get());

    EXPECT_EQ(grammar->getNonTerminals().size(), ntCountBefore);

    NTListItem* a = grammar->getNTItem("A");
    NTListItem* b = grammar->getNTItem("B");
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, grammar.get()));
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(b, grammar.get()));
}

TEST_F(RightEliminationTest, PureRightRecursion_NoBase) {
    // A : 'a' , A.  — only recursive, no base case
    // RA = 'a', RB = nil → newRoot = @ * 'a'  (zero or more 'a')
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A.");

    NTListItem* a = grammar->getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(a, grammar.get()));

    EXPECT_NO_THROW(RightElimination::eliminateForNonTerminal(a, grammar.get()));
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, grammar.get()));
    EXPECT_TRUE(a->hasRoot());
}

TEST_F(RightEliminationTest, NoRecursion_NotModified) {
    // S : 'a' , 'b'.  — no recursion, eliminate should not crash or modify
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b'.");

    NTListItem* s = grammar->getNTItem("S");
    std::string ruleBefore = s->value();

    RightElimination::eliminateForNonTerminal(s, grammar.get());

    // eliminateForNonTerminal skips if no right recursion
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(s, grammar.get()));
    EXPECT_EQ(s->value(), ruleBefore);
}

TEST_F(RightEliminationTest, EpsilonInAlternative) {
    // A : 'a' , A ; @.  — right-recursive with epsilon alternative
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; @.");

    NTListItem* a = grammar->getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(a, grammar.get()));

    EXPECT_NO_THROW(RightElimination::eliminateForNonTerminal(a, grammar.get()));
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, grammar.get()));
    EXPECT_TRUE(a->hasRoot());
}

TEST_F(RightEliminationTest, SaveAndLoadAfterElimination) {
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; 'b'.");

    RightElimination::eliminate(grammar.get());

    std::string filename = "test_right_elim.grm";
    grammar->save(filename);

    Grammar grammar2;
    grammar2.load(filename);

    NTListItem* a = grammar2.getNTItem("A");
    ASSERT_NE(a, nullptr);
    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(a, &grammar2));
    EXPECT_EQ(grammar2.getNTItem("A_rec"), nullptr);
    EXPECT_TRUE(a->hasRoot());

    std::remove(filename.c_str());
}

TEST_F(RightEliminationTest, SymmetryWithLeftElimination) {
    // Right-recursive grammar: same language as the left-recursive E : E,'+',term ; term
    // List : 'item' , List ; @.  →  after eliminate: @ * 'item'  (zero or more items)
    grammar->addNonTerminal("List");
    grammar->setNTRule("List", "'item' , List ; @.");

    NTListItem* list = grammar->getNTItem("List");
    ASSERT_NE(list, nullptr);
    EXPECT_TRUE(RightElimination::hasDirectRightRecursion(list, grammar.get()));

    RightElimination::eliminateForNonTerminal(list, grammar.get());

    EXPECT_FALSE(RightElimination::hasDirectRightRecursion(list, grammar.get()));
    EXPECT_TRUE(list->hasRoot());
    // Result should contain 'item' in a star context
    std::string rule = list->value();
    EXPECT_NE(rule.find("item"), std::string::npos);
    EXPECT_NE(rule.find('*'), std::string::npos);
}
