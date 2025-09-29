#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/analysis/ParsingTable.h>
#include <syngt/codegen/ParserGenerator.h>
#include <fstream>

using namespace syngt;

class ParserGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(ParserGeneratorTest, GenerateCPPCode) {
    grammar->setNTRule("S", "'a'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    std::string code = ParserGenerator::generate(
        grammar.get(),
        table.get(),
        ParserGenerator::Language::CPP,
        "TestParser"
    );
    
    EXPECT_FALSE(code.empty());
    EXPECT_NE(code.find("class TestParser"), std::string::npos);
    EXPECT_NE(code.find("TokenType"), std::string::npos);
    EXPECT_NE(code.find("NonTerminal"), std::string::npos);
}

TEST_F(ParserGeneratorTest, GeneratePythonCode) {
    grammar->setNTRule("S", "'a'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    std::string code = ParserGenerator::generate(
        grammar.get(),
        table.get(),
        ParserGenerator::Language::Python,
        "TestParser"
    );
    
    EXPECT_FALSE(code.empty());
    EXPECT_NE(code.find("class TestParser"), std::string::npos);
    EXPECT_NE(code.find("TOKEN_"), std::string::npos);
    EXPECT_NE(code.find("def parse"), std::string::npos);
    EXPECT_NE(code.find("Symbol"), std::string::npos);
}

TEST_F(ParserGeneratorTest, SaveToFile) {
    grammar->setNTRule("S", "'a' , 'b'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    std::string filename = "generated_parser.cpp";
    
    EXPECT_NO_THROW(ParserGenerator::saveToFile(
        grammar.get(),
        table.get(),
        ParserGenerator::Language::CPP,
        filename,
        "GeneratedParser"
    ));
    
    // Проверяем что файл создан
    std::ifstream file(filename);
    EXPECT_TRUE(file.good());
    file.close();
    
    std::remove(filename.c_str());
}

TEST_F(ParserGeneratorTest, GenerateComplexGrammar) {
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
    
    std::string code = ParserGenerator::generate(
        grammar.get(),
        table.get(),
        ParserGenerator::Language::CPP,
        "ArithmeticParser"
    );
    
    EXPECT_FALSE(code.empty());
    
    // Проверяем что все нетерминалы присутствуют
    EXPECT_NE(code.find("E"), std::string::npos);
    EXPECT_NE(code.find("E_prime"), std::string::npos);
    EXPECT_NE(code.find("T"), std::string::npos);
}

TEST_F(ParserGeneratorTest, GenerateWorkingParser) {
    // Простая грамматика: S → 'a' 'b'
    grammar->setNTRule("S", "'a' , 'b'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    EXPECT_FALSE(table->hasConflicts());
    
    std::string code = ParserGenerator::generate(
        grammar.get(),
        table.get(),
        ParserGenerator::Language::CPP,
        "SimpleParser"
    );
    
    // Проверяем ключевые элементы полного парсера
    EXPECT_NE(code.find("std::stack<Symbol> parseStack"), std::string::npos);
    EXPECT_NE(code.find("parsingTable"), std::string::npos);
    EXPECT_NE(code.find("for (int i = prod.size() - 1; i >= 0; i--)"), std::string::npos);
    EXPECT_NE(code.find("parseStack.push(prod[i])"), std::string::npos);
}

TEST_F(ParserGeneratorTest, GenerateWithProductions) {
    // Грамматика с несколькими продукциями
    // S → A | B
    // A → 'a'
    // B → 'b'
    
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "A ; B.");
    grammar->setNTRule("A", "'a'.");
    grammar->setNTRule("B", "'b'.");
    
    auto table = ParsingTable::build(grammar.get());
    ASSERT_NE(table, nullptr);
    
    std::string code = ParserGenerator::generate(
        grammar.get(),
        table.get(),
        ParserGenerator::Language::CPP,
        "MultiParser"
    );
    
    // Проверяем что продукции инициализируются
    EXPECT_NE(code.find("initParsingTable"), std::string::npos);
    EXPECT_NE(code.find("parsingTable[{"), std::string::npos);
}