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
    // S → 'if' expr 'then' stmt | 'if' expr 'then' stmt 'else' stmt
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'if' , expr , 'then' , stmt ; 'if' , expr , 'then' , stmt , 'else' , stmt.");
    
    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    ASSERT_TRUE(s->hasRoot());
}

TEST_F(LeftFactorizationTest, NoCommonPrefix) {
    // S → 'a' | 'b'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' ; 'b'.");
    
    NTListItem* s = grammar->getNTItem("S");
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(s, grammar.get());
    
    // Не должно измениться (нет общих префиксов)
    EXPECT_EQ(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, FactorizeSimple) {
    // S → 'a' 'b' | 'a' 'c'
    // Должно стать: S → 'a' S', S' → 'b' | 'c'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    // Должен добавиться новый нетерминал
    EXPECT_GT(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, FactorizeMultiple) {
    // S → 'x' 'a' | 'x' 'b' | 'x' 'c'
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'x' , 'a' ; 'x' , 'b' ; 'x' , 'c'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    EXPECT_GT(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, FactorizeAllGrammar) {
    // Создаем грамматику с несколькими правилами, требующими факторизации
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("A", "'a' , 'b' ; 'a' , 'c'.");
    grammar->setNTRule("B", "'x' , 'y' ; 'x' , 'z'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorizeAll(grammar.get());
    
    // Должны добавиться новые нетерминалы для факторизованных правил
    EXPECT_GT(grammar->getNonTerminals().size(), beforeCount);
}

TEST_F(LeftFactorizationTest, SaveAfterFactorization) {
    grammar->addNonTerminal("stmt");
    grammar->setNTRule("stmt", "'if' , cond , 'then' , stmt ; 'if' , cond , 'then' , stmt , 'else' , stmt.");
    
    LeftFactorization::factorizeAll(grammar.get());
    
    std::string filename = "test_factorized.grm";
    EXPECT_NO_THROW(grammar->save(filename));
    
    Grammar grammar2;
    EXPECT_NO_THROW(grammar2.load(filename));
    
    EXPECT_GE(grammar2.getNonTerminals().size(), grammar->getNonTerminals().size());
    
    std::remove(filename.c_str());
}