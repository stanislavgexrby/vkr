#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REIteration.h>

using namespace syngt;

class REOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(REOperationsTest, DeepNesting) {
    // ((('a' , 'b') ; 'c') , 'd')
    int aId = grammar->addTerminal("a");
    int bId = grammar->addTerminal("b");
    int cId = grammar->addTerminal("c");
    int dId = grammar->addTerminal("d");
    
    auto a = std::make_unique<RETerminal>(grammar.get(), aId);
    auto b = std::make_unique<RETerminal>(grammar.get(), bId);
    auto ab = REAnd::make(std::move(a), std::move(b));
    
    auto c = std::make_unique<RETerminal>(grammar.get(), cId);
    auto abc = REOr::make(std::move(ab), std::move(c));
    
    auto d = std::make_unique<RETerminal>(grammar.get(), dId);
    auto final = REAnd::make(std::move(abc), std::move(d));
    
    std::string result = final->toString(EmptyMask(), false);
    EXPECT_NE(result.find("'a'"), std::string::npos);
    EXPECT_NE(result.find("'b'"), std::string::npos);
    EXPECT_NE(result.find("'c'"), std::string::npos);
    EXPECT_NE(result.find("'d'"), std::string::npos);
}

TEST_F(REOperationsTest, MultipleIterations) {
    // 'a' * ('b' * 'c')
    int aId = grammar->addTerminal("a");
    int bId = grammar->addTerminal("b");
    int cId = grammar->addTerminal("c");
    
    auto a = std::make_unique<RETerminal>(grammar.get(), aId);
    auto b = std::make_unique<RETerminal>(grammar.get(), bId);
    auto c = std::make_unique<RETerminal>(grammar.get(), cId);
    
    auto bc_iter = REIteration::make(std::move(b), std::move(c));
    auto final = REIteration::make(std::move(a), std::move(bc_iter));
    
    std::string result = final->toString(EmptyMask(), false);
    EXPECT_NE(result.find('*'), std::string::npos);
}

TEST_F(REOperationsTest, MixedNonTerminalsAndTerminals) {
    // A , 'plus' , B
    int plusId = grammar->addTerminal("plus");
    int aId = grammar->addNonTerminal("A");
    int bId = grammar->addNonTerminal("B");
    
    auto a = std::make_unique<RENonTerminal>(grammar.get(), aId);
    auto plus = std::make_unique<RETerminal>(grammar.get(), plusId);
    auto b = std::make_unique<RENonTerminal>(grammar.get(), bId);
    
    auto a_plus = REAnd::make(std::move(a), std::move(plus));
    auto final = REAnd::make(std::move(a_plus), std::move(b));
    
    std::string result = final->toString(EmptyMask(), false);
    EXPECT_NE(result.find("A"), std::string::npos);
    EXPECT_NE(result.find("'plus'"), std::string::npos);
    EXPECT_NE(result.find("B"), std::string::npos);
}

TEST_F(REOperationsTest, SemanticsInExpression) {
    // 'a' , @action , 'b'
    int aId = grammar->addTerminal("a");
    int bId = grammar->addTerminal("b");
    int actionId = grammar->addSemantic("@action");
    
    auto a = std::make_unique<RETerminal>(grammar.get(), aId);
    auto action = std::make_unique<RESemantic>(grammar.get(), actionId);
    auto b = std::make_unique<RETerminal>(grammar.get(), bId);
    
    auto a_action = REAnd::make(std::move(a), std::move(action));
    auto final = REAnd::make(std::move(a_action), std::move(b));
    
    std::string result = final->toString(EmptyMask(), false);
    EXPECT_NE(result.find("@action"), std::string::npos);
}

TEST_F(REOperationsTest, EmptyAlternative) {
    // 'a' | @
    int aId = grammar->addTerminal("a");
    
    auto a = std::make_unique<RETerminal>(grammar.get(), aId);
    auto empty = std::make_unique<RESemantic>(grammar.get(), 0);  // @ is semantic with id 0
    
    auto final = REOr::make(std::move(a), std::move(empty));
    
    std::string result = final->toString(EmptyMask(), false);
    EXPECT_NE(result.find("'a'"), std::string::npos);
}

TEST_F(REOperationsTest, CopyPreservesStructure) {
    int aId = grammar->addTerminal("a");
    int bId = grammar->addTerminal("b");
    
    auto a = std::make_unique<RETerminal>(grammar.get(), aId);
    auto b = std::make_unique<RETerminal>(grammar.get(), bId);
    auto original = REOr::make(std::move(a), std::move(b));
    
    std::string originalStr = original->toString(EmptyMask(), false);
    auto copy = original->copy();
    std::string copyStr = copy->toString(EmptyMask(), false);
    
    EXPECT_EQ(originalStr, copyStr);
}