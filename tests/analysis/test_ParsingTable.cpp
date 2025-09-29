#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/analysis/ParsingTable.h>

using namespace syngt;

class ParsingTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(ParsingTableTest, BuildSimpleTable) {
    // S → 'a'
    grammar->setNTRule("S", "'a'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    int aId = grammar->findTerminal("a");
    EXPECT_TRUE(table->hasRule("S", aId));
}

TEST_F(ParsingTableTest, BuildWithAlternatives) {
    // S → 'a' | 'b'
    grammar->setNTRule("S", "'a' ; 'b'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    int aId = grammar->findTerminal("a");
    int bId = grammar->findTerminal("b");
    
    EXPECT_TRUE(table->hasRule("S", aId));
    EXPECT_TRUE(table->hasRule("S", bId));
}

TEST_F(ParsingTableTest, NoConflictsLL1Grammar) {
    // LL(1) грамматика
    // S → 'a' A | 'b' B
    // A → 'c'
    // B → 'd'
    
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "'a' , A ; 'b' , B.");
    grammar->setNTRule("A", "'c'.");
    grammar->setNTRule("B", "'d'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    EXPECT_FALSE(table->hasConflicts());
}

TEST_F(ParsingTableTest, DetectConflicts) {
    // Не LL(1): обе альтернативы начинаются с 'a'
    // S → 'a' 'b' | 'a' 'c'
    
    grammar->setNTRule("S", "'a' , 'b' ; 'a' , 'c'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    // Должен быть конфликт в ячейке M[S, 'a']
    EXPECT_TRUE(table->hasConflicts());
    EXPECT_GT(table->getConflicts().size(), 0);
}

TEST_F(ParsingTableTest, PrintDoesNotCrash) {
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A , 'b'.");
    grammar->setNTRule("A", "'a'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    EXPECT_NO_THROW(table->print(grammar.get()));
}

TEST_F(ParsingTableTest, ExportForCodegen) {
    grammar->setNTRule("S", "'a'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    std::string exported = table->exportForCodegen(grammar.get());
    EXPECT_FALSE(exported.empty());
    EXPECT_NE(exported.find("ParsingTable"), std::string::npos);
}

TEST_F(ParsingTableTest, ComplexGrammar) {
    // E → T E'
    // E' → '+' T E' | @
    // T → 'num'
    
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("E_prime");
    grammar->addNonTerminal("T");
    
    grammar->setNTRule("E", "T , E_prime.");
    grammar->setNTRule("E_prime", "'+' , T , E_prime ; @.");
    grammar->setNTRule("T", "'num'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    // Проверяем что таблица построена без конфликтов (это LL(1) грамматика)
    EXPECT_FALSE(table->hasConflicts());
    
    // Проверяем некоторые ячейки
    int numId = grammar->findTerminal("num");
    int plusId = grammar->findTerminal("+");
    
    EXPECT_TRUE(table->hasRule("E", numId));
    EXPECT_TRUE(table->hasRule("E_prime", plusId));
    EXPECT_TRUE(table->hasRule("T", numId));
}