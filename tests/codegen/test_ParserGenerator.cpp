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
    EXPECT_NE(code.find("TokenType"), std::string::npos);
    EXPECT_NE(code.find("def parse"), std::string::npos);
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