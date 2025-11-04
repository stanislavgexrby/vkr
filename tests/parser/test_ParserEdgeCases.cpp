#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>

using namespace syngt;

class ParserEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        parser = std::make_unique<Parser>();
    }
    
    std::unique_ptr<Grammar> grammar;
    std::unique_ptr<Parser> parser;
};

TEST_F(ParserEdgeCasesTest, EmptyRule) {
    auto tree = parser->parse("@.", grammar.get());
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->toString(EmptyMask(), false), "@");
}

TEST_F(ParserEdgeCasesTest, SingleTerminal) {
    auto tree = parser->parse("'x'.", grammar.get());
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->toString(EmptyMask(), false), "'x'");
}

TEST_F(ParserEdgeCasesTest, SingleNonTerminal) {
    grammar->addNonTerminal("A");
    auto tree = parser->parse("A.", grammar.get());
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->toString(EmptyMask(), false), "A");
}

TEST_F(ParserEdgeCasesTest, LongSequence) {
    auto tree = parser->parse("'a' , 'b' , 'c' , 'd' , 'e'.", grammar.get());
    ASSERT_NE(tree, nullptr);
    
    std::string result = tree->toString(EmptyMask(), false);
    EXPECT_NE(result.find("'a'"), std::string::npos);
    EXPECT_NE(result.find("'e'"), std::string::npos);
}

TEST_F(ParserEdgeCasesTest, LongAlternative) {
    auto tree = parser->parse("'a' ; 'b' ; 'c' ; 'd' ; 'e'.", grammar.get());
    ASSERT_NE(tree, nullptr);
    
    std::string result = tree->toString(EmptyMask(), false);
    EXPECT_NE(result.find(';'), std::string::npos);
}

TEST_F(ParserEdgeCasesTest, NestedParentheses) {
    auto tree = parser->parse("(('a' , 'b') ; ('c' , 'd')).", grammar.get());
    ASSERT_NE(tree, nullptr);
}

TEST_F(ParserEdgeCasesTest, WithWhitespace) {
    auto tree = parser->parse("  'a'   ,   'b'   .  ", grammar.get());
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->toString(EmptyMask(), false), "'a','b'");
}

TEST_F(ParserEdgeCasesTest, SpecialCharactersInTerminals) {
    auto tree = parser->parse("'+' , '-' , '*' , '/'.", grammar.get());
    ASSERT_NE(tree, nullptr);
}

TEST_F(ParserEdgeCasesTest, IterationWithParentheses) {
    auto tree = parser->parse("'a' * ('b' , 'c').", grammar.get());
    ASSERT_NE(tree, nullptr);
    
    std::string result = tree->toString(EmptyMask(), false);
    EXPECT_NE(result.find('*'), std::string::npos);
}

TEST_F(ParserEdgeCasesTest, ComplexMixed) {
    grammar->addNonTerminal("expr");
    grammar->addNonTerminal("term");
    
    auto tree = parser->parse("expr , ('+' ; '-') , term.", grammar.get());
    ASSERT_NE(tree, nullptr);
}

TEST_F(ParserEdgeCasesTest, SemanticsWithActions) {
    auto tree = parser->parse("'a' , @action1 , 'b' , @action2.", grammar.get());
    ASSERT_NE(tree, nullptr);
    
    std::string result = tree->toString(EmptyMask(), false);
    EXPECT_NE(result.find("@action"), std::string::npos);
}

TEST_F(ParserEdgeCasesTest, ErrorHandlingMissingDot) {
    EXPECT_ANY_THROW(parser->parse("'a' , 'b'", grammar.get()));
}

TEST_F(ParserEdgeCasesTest, ErrorHandlingUnbalancedParens) {
    EXPECT_ANY_THROW(parser->parse("('a' , 'b'.", grammar.get()));
}

TEST_F(ParserEdgeCasesTest, DoubleQuotedTerminals) {
    auto tree = parser->parse("\"hello\" , \"world\".", grammar.get());
    ASSERT_NE(tree, nullptr);
}

TEST_F(ParserEdgeCasesTest, MixedQuotes) {
    auto tree = parser->parse("'single' , \"double\".", grammar.get());
    ASSERT_NE(tree, nullptr);
}