#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/transform/LeftFactorization.h>

using namespace syngt;

class LeftFactorizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(LeftFactorizationTest, DetectCommonPrefix) {
    // S : 'if' expr 'then' stmt | 'if' expr 'then' stmt 'else' stmt
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'if' , expr , 'then' , stmt ; 'if' , expr , 'then' , stmt , 'else' , stmt.");
    
    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    ASSERT_TRUE(s->hasRoot());
}

TEST_F(LeftFactorizationTest, NoCommonPrefix) {
    // S : 'a' | 'b'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' ; 'b'.");
    
    NTListItem* s = grammar->getNTItem("S");
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(s, grammar.get());
    
    EXPECT_EQ(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, FactorizeSimple) {
    // S : 'a' 'b' | 'a' 'c'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    EXPECT_GT(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, FactorizeMultiple) {
    // S : 'x' 'a' | 'x' 'b' | 'x' 'c'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'x' , 'a' ; 'x' , 'b' ; 'x' , 'c'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    EXPECT_GT(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, FactorizeAllGrammar) {
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("A", "'a' , 'b' ; 'a' , 'c'.");
    grammar->setNTRule("B", "'x' , 'y' ; 'x' , 'z'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorizeAll(grammar.get());
    
    EXPECT_GT(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, SaveAfterFactorization) {
    grammar->addNonTerminal("stmt");
    grammar->setNTRule("stmt", "'if' , cond , 'then' , stmt ; 'if' , cond , 'then' , stmt , 'else' , stmt.");
    
    LeftFactorization::factorize(grammar->getNTItem("stmt"), grammar.get());
    
    std::string filename = "test_factorized.grm";
    grammar->save(filename);
    
    Grammar grammar2;
    grammar2.load(filename);
    
    EXPECT_GE(grammar2.getNonTerminals().size(), grammar->getNonTerminals().size());
    
    std::remove(filename.c_str());
}

TEST_F(LeftFactorizationTest, RecursiveFactorization) {
    // S : 'a' 'b' 'c' | 'a' 'b' 'd' | 'a' 'x'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c' ; 'a' , 'b' , 'd' ; 'a' , 'x'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    std::cout << "Before: " << beforeCount << " NTs\n";
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    size_t afterCount = grammar->getNonTerminals().size();
    std::cout << "After: " << afterCount << " NTs\n";
    
    EXPECT_GT(afterCount, beforeCount);
    
    NTListItem* s = grammar->getNTItem("S");
    EXPECT_TRUE(s->hasRoot());
    
    std::cout << "Final S rule: " << s->value() << "\n";
}

TEST_F(LeftFactorizationTest, DeepRecursion) {
    // S : 'a' 'b' 'c' 'd' | 'a' 'b' 'c' 'e' | 'a' 'b' 'f'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c' , 'd' ; 'a' , 'b' , 'c' , 'e' ; 'a' , 'b' , 'f'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    size_t afterCount = grammar->getNonTerminals().size();
    
    EXPECT_GT(afterCount, beforeCount);
    
    NTListItem* s = grammar->getNTItem("S");
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(LeftFactorizationTest, NoInfiniteLoop) {
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    EXPECT_NO_THROW(LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get()));
}