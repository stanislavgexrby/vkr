#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>

using namespace syngt;

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        parser = std::make_unique<Parser>();
    }
    
    std::unique_ptr<Grammar> grammar;
    std::unique_ptr<Parser> parser;
};

TEST_F(ParserTest, SimpleSequence) {
    auto tree = parser->parse("begin , end.", grammar.get());
    EXPECT_EQ(tree->toString(EmptyMask(), false), "begin,end");
}

TEST_F(ParserTest, SimpleAlternative) {
    auto tree = parser->parse("'program' ; 'begin'.", grammar.get());
    EXPECT_EQ(tree->toString(EmptyMask(), false), "'program';'begin'");
}

TEST_F(ParserTest, Iteration) {
    auto tree = parser->parse("statement * expression.", grammar.get());
    EXPECT_EQ(tree->toString(EmptyMask(), false), "statement*expression");
}

TEST_F(ParserTest, WithParentheses) {
    auto tree = parser->parse("( 'begin' , statement ) ; 'end'.", grammar.get());
    EXPECT_EQ(tree->toString(EmptyMask(), false), "'begin',statement;'end'");
}

TEST_F(ParserTest, ComplexExpression) {
    auto tree = parser->parse("'use' , '(' , library , @*( ',' , library ) , ')'.", grammar.get());
    EXPECT_FALSE(tree->toString(EmptyMask(), false).empty());
}