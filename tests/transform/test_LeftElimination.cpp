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

TEST_F(LeftEliminationTest, DetectDirectLeftRecursion) {
    // E → E '+' term | term
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , term ; term.");
    
    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    
    // Должна обнаружить левую рекурсию
    bool hasRecursion = LeftElimination::hasDirectLeftRecursion(e);
    EXPECT_TRUE(hasRecursion);
}

TEST_F(LeftEliminationTest, NoLeftRecursion) {
    // E → term '+' E | term
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "term , '+' , E ; term.");
    
    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    
    // Нет левой рекурсии (это правая рекурсия)
    bool hasRecursion = LeftElimination::hasDirectLeftRecursion(e);
    EXPECT_FALSE(hasRecursion);
}

TEST_F(LeftEliminationTest, EliminateSimpleLeftRecursion) {
    // E → E '+' term | term
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("term");
    grammar->setNTRule("E", "E , '+' , term ; term.");
    
    NTListItem* e = grammar->getNTItem("E");
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(LeftElimination::hasDirectLeftRecursion(e));
    
    // Устраняем левую рекурсию
    LeftElimination::eliminateForNonTerminal(e, grammar.get());
    
    // После трансформации не должно быть левой рекурсии
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    
    // Проверяем, что создался новый нетерминал E_rec
    NTListItem* eRec = grammar->getNTItem("E_rec");
    EXPECT_NE(eRec, nullptr);
    
    // Проверяем структуру: E → term E_rec
    EXPECT_TRUE(e->hasRoot());
    std::string eRule = e->value();
    EXPECT_NE(eRule.find("term"), std::string::npos);
    EXPECT_NE(eRule.find("E_rec"), std::string::npos);
}

TEST_F(LeftEliminationTest, EliminateMultipleAlternatives) {
    // E → E '+' | E '-' | num
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' ; E , '-' ; num.");
    
    LeftElimination::eliminateForNonTerminal(grammar->getNTItem("E"), grammar.get());
    
    NTListItem* e = grammar->getNTItem("E");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    
    // Должен создаться E_rec
    EXPECT_NE(grammar->getNTItem("E_rec"), nullptr);
}

TEST_F(LeftEliminationTest, EliminateForWholeGrammar) {
    // Создаем грамматику с несколькими левыми рекурсиями
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("T");
    grammar->setNTRule("E", "E , '+' , T ; T.");
    grammar->setNTRule("T", "T , '*' , F ; F.");
    
    // Устраняем все
    LeftElimination::eliminate(grammar.get());
    
    // Проверяем E
    NTListItem* e = grammar->getNTItem("E");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(e));
    EXPECT_NE(grammar->getNTItem("E_rec"), nullptr);
    
    // Проверяем T
    NTListItem* t = grammar->getNTItem("T");
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(t));
    EXPECT_NE(grammar->getNTItem("T_rec"), nullptr);
}

TEST_F(LeftEliminationTest, SaveAndLoadAfterElimination) {
    // Создаем грамматику с левой рекурсией
    grammar->addNonTerminal("expr");
    grammar->setNTRule("expr", "expr , '+' , term ; term.");
    
    // Устраняем
    LeftElimination::eliminate(grammar.get());
    
    // Сохраняем
    std::string filename = "test_left_elim.grm";
    grammar->save(filename);
    
    // Загружаем обратно
    Grammar grammar2;
    grammar2.load(filename);
    
    // Проверяем, что левой рекурсии нет
    NTListItem* expr = grammar2.getNTItem("expr");
    ASSERT_NE(expr, nullptr);
    EXPECT_FALSE(LeftElimination::hasDirectLeftRecursion(expr));
    
    // Проверяем наличие _rec нетерминала
    EXPECT_NE(grammar2.getNTItem("expr_rec"), nullptr);
    
    std::remove(filename.c_str());
}