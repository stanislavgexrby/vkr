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

// Полный цикл: парсинг → трансформация → визуализация
TEST_F(IntegrationTest, ParseTransformVisualize) {
    // 1. Добавляем правило с левой рекурсией
    grammar->addNonTerminal("E");
    grammar->addTerminal("+");
    grammar->addTerminal("n");
    
    // E -> E + n ; n
    grammar->setNTRule("E", "E , '+' , 'n' ; 'n'.");
    
    // 2. Устраняем левую рекурсию
    LeftElimination::eliminate(grammar.get());
    
    // 3. Проверяем что правило изменилось
    auto item = grammar->getNTItem("E");
    ASSERT_TRUE(item != nullptr);
    ASSERT_TRUE(item->hasRoot());
    
    // 4. Создаем визуализацию
    auto list = std::make_unique<DrawObjectList>(grammar.get());
    Creator::createDrawObjects(list.get(), item->root());
    
    EXPECT_GT(list->count(), 0);
    EXPECT_GT(list->width(), 0);
}

// Цикл: загрузка → факторизация → сохранение
TEST_F(IntegrationTest, LoadFactorizeSave) {
    // 1. Создаем грамматику
    grammar->addNonTerminal("S");
    grammar->addTerminal("a");
    grammar->addTerminal("b");
    grammar->addTerminal("c");
    
    // S -> a b ; a c (есть общий префикс 'a')
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");

    // 2. Факторизация
    auto item = grammar->getNTItem("S");
    LeftFactorization::factorize(item, grammar.get());

    // 3. Проверяем результат (item уже объявлен выше)
    ASSERT_TRUE(item != nullptr);
    ASSERT_TRUE(item->hasRoot());
}

// Цикл: множественные трансформации
TEST_F(IntegrationTest, MultipleTransformations) {
    // 1. Создаем сложную грамматику
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    grammar->addTerminal("x");
    grammar->addTerminal("y");
    
    grammar->setNTRule("A", "B , 'x' ; C.");
    grammar->setNTRule("B", "'y'.");
    grammar->setNTRule("C", "'@'.");
    
    // 2. Удаляем бесполезные символы
    RemoveUseless::remove(grammar.get());
    
    // 3. Проверяем что все нужные символы остались
    EXPECT_TRUE(grammar->getNTItem("A")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("B")->hasRoot());
}

// Работа с несколькими нетерминалами
TEST_F(IntegrationTest, MultipleNonTerminals) {
    // Создаем несколько связанных правил
    grammar->addNonTerminal("program");
    grammar->addNonTerminal("statement");
    grammar->addNonTerminal("expression");
    grammar->addTerminal("begin");
    grammar->addTerminal("end");
    grammar->addTerminal("id");
    
    grammar->setNTRule("program", "'begin' , statement , 'end'.");
    grammar->setNTRule("statement", "expression.");
    grammar->setNTRule("expression", "'id'.");
    
    // Проверяем все правила
    EXPECT_TRUE(grammar->getNTItem("program")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("statement")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("expression")->hasRoot());
    
    // Создаем визуализацию для каждого
    auto list = std::make_unique<DrawObjectList>(grammar.get());
    
    for (const auto& ntName : grammar->getNonTerminals()) {
        auto item = grammar->getNTItem(ntName);
        if (item && item->hasRoot()) {
            Creator::createDrawObjects(list.get(), item->root());
            EXPECT_GT(list->count(), 0);
        }
    }
}

// Стресс-тест: большая грамматика
TEST_F(IntegrationTest, LargeGrammar) {
    // Создаем 20 нетерминалов
    for (int i = 0; i < 20; i++) {
        grammar->addNonTerminal("NT" + std::to_string(i));
    }
    
    // Создаем 30 терминалов
    for (int i = 0; i < 30; i++) {
        grammar->addTerminal("term" + std::to_string(i));
    }
    
    // Устанавливаем правила
    for (int i = 0; i < 20; i++) {
        std::string rule = "'term" + std::to_string(i) + "'.";
        grammar->setNTRule("NT" + std::to_string(i), rule);
    }
    
    // Проверяем что все добавилось
    EXPECT_EQ(grammar->getNonTerminals().size(), 21); // +1 для S
    EXPECT_GE(grammar->getTerminals().size(), 30);
}

// Проверка копирования грамматики
TEST_F(IntegrationTest, GrammarCloning) {
    grammar->addTerminal("token");
    grammar->addNonTerminal("rule");
    grammar->setNTRule("rule", "'token'.");
    
    // Создаем копию правила
    auto item = grammar->getNTItem("rule");
    ASSERT_TRUE(item != nullptr);
    
    auto copiedTree = item->copyRETree();
    EXPECT_TRUE(copiedTree != nullptr);
    
    // Проверяем что копия идентична
    EXPECT_EQ(item->root()->toString(SelectionMask(), false),
              copiedTree->toString(SelectionMask(), false));
}