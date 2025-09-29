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

TEST_F(LeftFactorizationTest, RecursiveFactorization) {
    // S → 'a' 'b' 'c' | 'a' 'b' 'd' | 'a' 'x'
    // Должна быть многоуровневая факторизация
    
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b' , 'c' ; 'a' , 'b' , 'd' ; 'a' , 'x'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    std::cout << "Before: " << beforeCount << " NTs\n";
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    size_t afterCount = grammar->getNonTerminals().size();
    std::cout << "After: " << afterCount << " NTs\n";
    
    // После факторизации правила "S → abc | abd | ax"
    // должно быть создано хотя бы 1 новый нетерминал (для факторизации)
    // Рекурсивная факторизация может создать еще больше
    EXPECT_GT(afterCount, beforeCount);  // Хотя бы 1 новый
    
    // Проверяем что S имеет правило
    NTListItem* s = grammar->getNTItem("S");
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->hasRoot());
    
    // Выводим итоговое правило для отладки
    std::cout << "Final S rule: " << s->value() << "\n";
}

TEST_F(LeftFactorizationTest, DeepRecursion) {
    // Тестируем глубокую вложенность общих префиксов
    // S → 'a' 'b' 'c' 'd' | 'a' 'b' 'c' 'e' | 'a' 'b' 'f'
    
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a','b','c','d' ; 'a','b','c','e' ; 'a','b','f'.");
    
    size_t beforeCount = grammar->getNonTerminals().size();
    
    LeftFactorization::factorize(grammar->getNTItem("S"), grammar.get());
    
    // Должно создаться несколько уровней факторизованных нетерминалов
    size_t afterCount = grammar->getNonTerminals().size();
    EXPECT_GT(afterCount, beforeCount);
    
    // Проверяем что факторизация завершилась без ошибок
    NTListItem* s = grammar->getNTItem("S");
    EXPECT_TRUE(s->hasRoot());
}

TEST_F(LeftFactorizationTest, NoInfiniteLoop) {
    // Проверяем что нет бесконечного цикла на сложных случаях
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'x','y','z' ; 'x','y','w' ; 'p','q'.");
    
    // Должно завершиться без зависания
    EXPECT_NO_THROW(LeftFactorization::factorize(grammar->getNTItem("A"), grammar.get()));
}