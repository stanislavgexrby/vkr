#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/transform/FirstFollow.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/regex/RESemantic.h>    

using namespace syngt;

class FirstFollowTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(FirstFollowTest, FirstSingleTerminal) {
    // S → 'a'
    grammar->setNTRule("S", "'a'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    ASSERT_TRUE(firstSets.count("S") > 0);
    EXPECT_EQ(firstSets["S"].size(), 1);
    
    int aId = grammar->findTerminal("a");
    EXPECT_TRUE(firstSets["S"].count(aId) > 0);
}

TEST_F(FirstFollowTest, FirstSequence) {
    // S → 'a' 'b' 'c'
    // FIRST(S) = {'a'}
    grammar->setNTRule("S", "'a' , 'b' , 'c'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    int aId = grammar->findTerminal("a");
    int bId = grammar->findTerminal("b");
    
    EXPECT_TRUE(firstSets["S"].count(aId) > 0);
    EXPECT_FALSE(firstSets["S"].count(bId) > 0);
}

TEST_F(FirstFollowTest, FirstAlternatives) {
    // S → 'a' | 'b' | 'c'
    // FIRST(S) = {'a', 'b', 'c'}
    grammar->setNTRule("S", "'a' ; 'b' ; 'c'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    int aId = grammar->findTerminal("a");
    int bId = grammar->findTerminal("b");
    int cId = grammar->findTerminal("c");
    
    EXPECT_TRUE(firstSets["S"].count(aId) > 0);
    EXPECT_TRUE(firstSets["S"].count(bId) > 0);
    EXPECT_TRUE(firstSets["S"].count(cId) > 0);
    EXPECT_EQ(firstSets["S"].size(), 3);
}

TEST_F(FirstFollowTest, FirstWithNonTerminal) {
    // S → A 'b'
    // A → 'a'
    // FIRST(A) = {'a'}
    // FIRST(S) = FIRST(A) = {'a'}
    
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A , 'b'.");
    grammar->setNTRule("A", "'a'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    int aId = grammar->findTerminal("a");
    int bId = grammar->findTerminal("b");
    
    // FIRST(A) = {'a'}
    EXPECT_TRUE(firstSets["A"].count(aId) > 0);
    EXPECT_EQ(firstSets["A"].size(), 1);
    
    // FIRST(S) = {'a'}
    EXPECT_TRUE(firstSets["S"].count(aId) > 0);
    EXPECT_FALSE(firstSets["S"].count(bId) > 0);
}

TEST_F(FirstFollowTest, FirstWithNullableNonTerminal) {
    // S → A 'b'
    // A → @
    
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A , 'b'.");
    
    int epsilonId = grammar->addSemantic("@");
    auto epsilonTree = std::make_unique<RESemantic>(grammar.get(), epsilonId);
    grammar->setNTRoot("A", std::move(epsilonTree));
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    int bId = grammar->findTerminal("b");
    
    EXPECT_TRUE(firstSets["S"].count(bId) > 0);
}

TEST_F(FirstFollowTest, FirstRecursiveGrammar) {
    // E → T E'
    // E' → '+' T E' | @
    // T → 'num'
    
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("E_prime");
    grammar->addNonTerminal("T");
    
    grammar->setNTRule("E", "T , E_prime.");
    grammar->setNTRule("E_prime", "'+' , T , E_prime ; @.");
    grammar->setNTRule("T", "'num'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    
    int numId = grammar->findTerminal("num");
    int plusId = grammar->findTerminal("+");
    
    // FIRST(T) = {'num'}
    EXPECT_TRUE(firstSets["T"].count(numId) > 0);
    
    // FIRST(E') = {'+', epsilon}
    EXPECT_TRUE(firstSets["E_prime"].count(plusId) > 0);
    
    // FIRST(E) = FIRST(T) = {'num'}
    EXPECT_TRUE(firstSets["E"].count(numId) > 0);
}

TEST_F(FirstFollowTest, FollowStartSymbol) {
    // S → 'a'
    // FOLLOW(S) = {$} (EOF)
    
    grammar->setNTRule("S", "'a'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    auto followSets = FirstFollow::computeFollow(grammar.get(), firstSets);
    
    ASSERT_TRUE(followSets.count("S") > 0);
    EXPECT_TRUE(followSets["S"].count(-1) > 0);
}

TEST_F(FirstFollowTest, FollowBasic) {
    // S → A 'b'
    // A → 'a'
    // FOLLOW(A) = {'b'}
    
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A , 'b'.");
    grammar->setNTRule("A", "'a'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    auto followSets = FirstFollow::computeFollow(grammar.get(), firstSets);
    
    int bId = grammar->findTerminal("b");
    
    EXPECT_TRUE(followSets["A"].count(bId) > 0);
}

TEST_F(FirstFollowTest, LL1Valid) {
    // S → 'a' A | 'b' B
    // A → 'c'
    // B → 'd'
    
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "'a' , A ; 'b' , B.");
    grammar->setNTRule("A", "'c'.");
    grammar->setNTRule("B", "'d'.");
    
    bool isLL1 = FirstFollow::isLL1(grammar.get());
    EXPECT_TRUE(isLL1);
}

TEST_F(FirstFollowTest, LL1Invalid_FirstConflict) {
    // S → 'a' 'b' | 'a' 'c'
    
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    bool isLL1 = FirstFollow::isLL1(grammar.get());
    EXPECT_FALSE(isLL1);
}

TEST_F(FirstFollowTest, LL1AfterFactorization) {
    // До: S → 'a' 'b' | 'a' 'c'  (не LL(1))
    
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    EXPECT_FALSE(FirstFollow::isLL1(grammar.get()));
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    bool isLL1After = FirstFollow::isLL1(grammar.get());
    EXPECT_TRUE(isLL1After);
}

TEST_F(FirstFollowTest, ComplexGrammar) {
    // S → A B
    // A → 'a' A | @
    // B → 'b' B | @
    
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "A , B.");
    grammar->setNTRule("A", "'a' , A ; @.");
    grammar->setNTRule("B", "'b' , B ; @.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    auto followSets = FirstFollow::computeFollow(grammar.get(), firstSets);
    
    int aId = grammar->findTerminal("a");
    int bId = grammar->findTerminal("b");
    
    // FIRST(A) = {'a', epsilon}
    EXPECT_TRUE(firstSets["A"].count(aId) > 0);
    
    // FIRST(B) = {'b', epsilon}
    EXPECT_TRUE(firstSets["B"].count(bId) > 0);
    
    // FIRST(S) = FIRST(A) ∪ FIRST(B) = {'a', 'b', epsilon}
    EXPECT_TRUE(firstSets["S"].count(aId) > 0 || firstSets["S"].count(bId) > 0);
}

TEST_F(FirstFollowTest, PrintSetsDoesNotCrash) {
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A , 'b'.");
    grammar->setNTRule("A", "'a'.");
    
    auto firstSets = FirstFollow::computeFirst(grammar.get());
    auto followSets = FirstFollow::computeFollow(grammar.get(), firstSets);
    
    EXPECT_NO_THROW(FirstFollow::printSets(grammar.get(), firstSets, followSets));
}