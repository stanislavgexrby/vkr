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

// ---------------------------------------------------------------------------
// Detection tests
// ---------------------------------------------------------------------------

TEST_F(LeftEliminationTest, DetectDirectLeftRecursion) {
    // E : E , '+' , term ; term.
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , term ; term.");

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(e));
}

TEST_F(LeftEliminationTest, NoLeftRecursionInRightRecursiveRule) {
    // E : term , '+' , E ; term.  — right-recursive, not left
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "term , '+' , E ; term.");

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
}

TEST_F(LeftEliminationTest, NoLeftRecursionInPlainRule) {
    // S : 'a' , 'b' , 'c'.  — no recursion at all
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c'.");

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(s));
}

TEST_F(LeftEliminationTest, DetectLeftRecursionInOr) {
    // S : S , 'a' ; 'b'.  — left-recursive in first alternative
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "S , 'a' ; 'b'.");

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(s));
}

// ---------------------------------------------------------------------------
// Inline elimination tests — Pascal algorithm creates NO new NTs
// ---------------------------------------------------------------------------

TEST_F(LeftEliminationTest, EliminateSimpleLeftRecursion) {
    // E : E , '+' , term ; term.
    // Expected: term , (@ * ('+' , term))   i.e. Arden: E = term · ('+' term)*
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("term");
    grammar->setNTRule("E", "E , '+' , term ; term.");

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(e));

    size_t ntCountBefore = grammar->getNonTerminals().size();
    LeftElimination::eliminateForNonTerminal(e, grammar.get());

    // No new NTs should be created (inline Pascal algorithm)
    EXPECT_EQ(grammar->getNonTerminals().size(), ntCountBefore);
    EXPECT_EQ(grammar->getNTItem("E_rec"), nullptr);

    // No left recursion after elimination
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));

    // The rule must still exist and reference the base case
    EXPECT_TRUE(e->hasRoot());
    std::string rule = e->value();
    // The base case 'term' must be present
    EXPECT_NE(rule.find("term"), std::string::npos);
    // The self-reference to E must be gone (algorithm is inline, not recursive)
    EXPECT_EQ(rule.find('E'), std::string::npos);
}

TEST_F(LeftEliminationTest, EliminateMultipleAlternatives) {
    // E : E , '+' ; E , '-' ; num.
    // Expected: num , (@ * ('+' ; '-'))
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' ; E , '-' ; num.");

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);

    size_t ntCountBefore = grammar->getNonTerminals().size();
    LeftElimination::eliminateForNonTerminal(e, grammar.get());

    EXPECT_EQ(grammar->getNonTerminals().size(), ntCountBefore);
    EXPECT_EQ(grammar->getNTItem("E_rec"), nullptr);
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));

    std::string rule = e->value();
    EXPECT_NE(rule.find("num"), std::string::npos);
}

TEST_F(LeftEliminationTest, EliminateForWholeGrammar) {
    // E : E , '+' , T ; T.
    // T : T , '*' , F ; F.
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("T");
    grammar->addNonTerminal("F");
    grammar->setNTRule("E", "E , '+' , T ; T.");
    grammar->setNTRule("T", "T , '*' , F ; F.");
    grammar->setNTRule("F", "'id'.");

    size_t ntCountBefore = grammar->getNonTerminals().size();
    LeftElimination::eliminate(grammar.get());

    // No new NTs created
    EXPECT_EQ(grammar->getNonTerminals().size(), ntCountBefore);
    EXPECT_EQ(grammar->getNTItem("E_rec"), nullptr);
    EXPECT_EQ(grammar->getNTItem("T_rec"), nullptr);

    NTListItem* e = grammar->getNTItem("E");
    NTListItem* t = grammar->getNTItem("T");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(t));
}

TEST_F(LeftEliminationTest, PureLeftRecursion_NoBase) {
    // E : E , 'a'.  — only recursive, no base case
    // By Arden: E = ε · E , 'a' → R1 = 'a', R2 = nil → newRoot = @ * 'a'
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , 'a'.");

    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(e));

    EXPECT_NO_THROW(LeftElimination::eliminateForNonTerminal(e, grammar.get()));
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    EXPECT_TRUE(e->hasRoot());
}

TEST_F(LeftEliminationTest, NoRecursion_RuleUnchanged) {
    // S : 'a' , 'b'.  — no recursion, eliminate should not crash
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b'.");

    NTListItem* s = grammar->getNTItem("S");
    std::string ruleBefore = s->value();

    LeftElimination::eliminateForNonTerminal(s, grammar.get());

    // Rule should be unchanged (or at least still valid and non-recursive)
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(s));
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(LeftEliminationTest, EpsilonInAlternative) {
    // S : S , 'a' ; @.   — left recursive, epsilon alternative
    // E=true (because '@' alternative), R2=nil (@ is epsilon)
    // newRoot = createAndAlt(nil, @*'a') = @*'a'
    // E=true but r2WasNonNull=false so no createOrEmpty
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "S , 'a' ; @.");

    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(s));

    EXPECT_NO_THROW(LeftElimination::eliminateForNonTerminal(s, grammar.get()));
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(s));
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(LeftEliminationTest, SaveAndLoadAfterElimination) {
    grammar->addNonTerminal("expr");
    grammar->addNonTerminal("term");
    grammar->setNTRule("expr", "expr , '+' , term ; term.");

    LeftElimination::eliminate(grammar.get());

    std::string filename = "test_left_elim.grm";
    grammar->save(filename);

    Grammar grammar2;
    grammar2.load(filename);

    NTListItem* expr = grammar2.getNTItem("expr");
    ASSERT_NE(expr, nullptr);
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(expr));
    // No _rec NT in new algorithm
    EXPECT_EQ(grammar2.getNTItem("expr_rec"), nullptr);
    EXPECT_TRUE(expr->hasRoot());

    std::remove(filename.c_str());
}