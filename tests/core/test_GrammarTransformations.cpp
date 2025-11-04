#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <syngt/transform/FirstFollow.h>

using namespace syngt;

class GrammarTransformationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(GrammarTransformationsTest, RegularizeSimpleGrammar) {
    // E : E '+' T | T
    // T : T '*' F | F
    // F : 'id'
    
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("T");
    grammar->addNonTerminal("F");
    
    grammar->setNTRule("E", "E , '+' , T ; T.");
    grammar->setNTRule("T", "T , '*' , F ; F.");
    grammar->setNTRule("F", "'id'.");
    
    EXPECT_NO_THROW(grammar->regularize());
    
    auto e = grammar->getNTItem("E");
    auto t = grammar->getNTItem("T");
    
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(t));
}

TEST_F(GrammarTransformationsTest, ChainedTransformations) {
    // S : 'a' 'b' | 'a' 'c'
    
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    size_t before = grammar->getNonTerminals().size();
    
    LeftFactorization::factorizeAll(grammar.get());
    
    size_t after = grammar->getNonTerminals().size();
    EXPECT_GT(after, before);
    
    EXPECT_NO_THROW(RemoveUseless::remove(grammar.get()));
}

TEST_F(GrammarTransformationsTest, EmptyProductionHandling) {
    // S : A B
    // A : @ | 'a'
    // B : 'b'
    
    grammar->addNonTerminal("S");
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "A , B.");
    grammar->setNTRule("A", "@ ; 'a'.");
    grammar->setNTRule("B", "'b'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    int bId = grammar->findTerminal("b");
    EXPECT_TRUE(firstSets["S"].count(bId) > 0);
}

TEST_F(GrammarTransformationsTest, IndirectLeftRecursion) {
    // S : A 'a' | 'b'
    // A : S 'c' | 'd'
    
    grammar->addNonTerminal("S");
    grammar->addNonTerminal("A");
    
    grammar->setNTRule("S", "A , 'a' ; 'b'.");
    grammar->setNTRule("A", "S , 'c' ; 'd'.");
    
    EXPECT_NO_THROW(LeftElimination::eliminate(grammar.get()));
}

TEST_F(GrammarTransformationsTest, MultipleLeftFactors) {
    // S : 'a' 'b' 'c' | 'a' 'b' 'd' | 'a' 'x' | 'y'
    
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c' ; 'a' , 'b' , 'd' ; 'a' , 'x' ; 'y'.");
    
    size_t before = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    size_t after = grammar->getNonTerminals().size();
    
    EXPECT_GT(after, before);
}

TEST_F(GrammarTransformationsTest, UnreachableNonTerminals) {
    // S : A
    // A : 'a'
    // B : 'b'
    // C : 'c'
    
    grammar->addNonTerminal("S");
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    
    grammar->setNTRule("S", "A.");
    grammar->setNTRule("A", "'a'.");
    grammar->setNTRule("B", "'b'.");
    grammar->setNTRule("C", "'c'.");
    
    RemoveUseless::remove(grammar.get());
    
    auto b = grammar->getNTItem("B");
    auto c = grammar->getNTItem("C");
    
    if (b) EXPECT_FALSE(b->hasRoot());
    if (c) EXPECT_FALSE(c->hasRoot());
}

TEST_F(GrammarTransformationsTest, NonProductiveNonTerminals) {
    // S : A
    // A : B 'a'
    // B : C
    // C : B
    
    grammar->addNonTerminal("S");
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    
    grammar->setNTRule("S", "A.");
    grammar->setNTRule("A", "B , 'a'.");
    grammar->setNTRule("B", "C.");
    grammar->setNTRule("C", "B.");
    
    RemoveUseless::remove(grammar.get());
    
    auto a = grammar->getNTItem("A");
    auto b = grammar->getNTItem("B");
    auto c = grammar->getNTItem("C");
    
    if (a) EXPECT_FALSE(a->hasRoot());
    if (b) EXPECT_FALSE(b->hasRoot());
    if (c) EXPECT_FALSE(c->hasRoot());
}

TEST_F(GrammarTransformationsTest, LL1Check) {
    // S : 'a' A | 'b' B
    // A : 'c'
    // B : 'd'
    
    grammar->addNonTerminal("S");
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "'a' , A ; 'b' , B.");
    grammar->setNTRule("A", "'c'.");
    grammar->setNTRule("B", "'d'.");
    
    EXPECT_TRUE(FirstFollow::isLL1(grammar.get()));
}

TEST_F(GrammarTransformationsTest, NotLL1Check) {
    // S : 'a' 'b' | 'a' 'c'
    
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    EXPECT_FALSE(FirstFollow::isLL1(grammar.get()));
}

TEST_F(GrammarTransformationsTest, MakeLL1ByFactorization) {
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    EXPECT_FALSE(FirstFollow::isLL1(grammar.get()));
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    EXPECT_TRUE(FirstFollow::isLL1(grammar.get()));
}

TEST_F(GrammarTransformationsTest, ComplexRealWorldGrammar) {
    // expr : expr '+' term | term
    // term : term '*' factor | factor
    // factor : '(' expr ')' | 'id'
    
    grammar->addNonTerminal("expr");
    grammar->addNonTerminal("term");
    grammar->addNonTerminal("factor");
    
    grammar->setNTRule("expr", "expr , '+' , term ; term.");
    grammar->setNTRule("term", "term , '*' , factor ; factor.");
    grammar->setNTRule("factor", "'(' , expr , ')' ; 'id'.");
    
    EXPECT_NO_THROW(grammar->regularize());
    
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(grammar->getNTItem("expr")));
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(grammar->getNTItem("term")));
}