#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REIteration.h>

using namespace syngt;

class RETreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        termId1 = grammar->addTerminal("a");
        termId2 = grammar->addTerminal("b");
    }
    
    std::unique_ptr<Grammar> grammar;
    int termId1;
    int termId2;
};

TEST_F(RETreeTest, TerminalCreation) {
    auto term = std::make_unique<RETerminal>(grammar.get(), termId1);
    
    EXPECT_EQ(term->id(), termId1);
    EXPECT_EQ(term->nameID(), "a");
    EXPECT_EQ(term->toString(EmptyMask(), false), "a");
}

TEST_F(RETreeTest, TerminalCopy) {
    auto term = std::make_unique<RETerminal>(grammar.get(), termId1);
    auto copy = term->copy();
    
    EXPECT_EQ(term->toString(EmptyMask(), false), 
              copy->toString(EmptyMask(), false));
}

TEST_F(RETreeTest, AndOperation) {
    auto term1 = std::make_unique<RETerminal>(grammar.get(), termId1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), termId2);
    
    auto andOp = REAnd::make(std::move(term1), std::move(term2));
    
    EXPECT_EQ(andOp->operationChar(), ',');
    EXPECT_EQ(andOp->toString(EmptyMask(), false), "a,b");
}

TEST_F(RETreeTest, OrOperation) {
    auto term1 = std::make_unique<RETerminal>(grammar.get(), termId1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), termId2);
    
    auto orOp = REOr::make(std::move(term1), std::move(term2));
    
    EXPECT_EQ(orOp->operationChar(), ';');
    EXPECT_EQ(orOp->toString(EmptyMask(), false), "a;b");
}

TEST_F(RETreeTest, IterationOperation) {
    auto term1 = std::make_unique<RETerminal>(grammar.get(), termId1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), termId2);
    
    auto iterOp = REIteration::make(std::move(term1), std::move(term2));
    
    EXPECT_EQ(iterOp->operationChar(), '*');
    EXPECT_EQ(iterOp->toString(EmptyMask(), false), "a*b");
}

TEST_F(RETreeTest, ComplexExpression) {
    // begin,(statement;expression),end
    int tBegin = grammar->addTerminal("begin");
    int tEnd = grammar->addTerminal("end");
    int ntStmt = grammar->addNonTerminal("statement");
    int ntExpr = grammar->addNonTerminal("expression");
    
    auto begin = std::make_unique<RETerminal>(grammar.get(), tBegin);
    auto stmt = std::make_unique<RENonTerminal>(grammar.get(), ntStmt, false);
    auto expr = std::make_unique<RENonTerminal>(grammar.get(), ntExpr, false);
    auto choice = REOr::make(std::move(stmt), std::move(expr));
    auto seq1 = REAnd::make(std::move(begin), std::move(choice));
    auto end = std::make_unique<RETerminal>(grammar.get(), tEnd);
    auto final = REAnd::make(std::move(seq1), std::move(end));
    
    EXPECT_EQ(final->toString(EmptyMask(), false), "begin,statement;expression,end");
    
    auto copy = final->copy();
    EXPECT_EQ(final->toString(EmptyMask(), false), 
              copy->toString(EmptyMask(), false));
}

TEST_F(RETreeTest, Semantics) {
    int semId = grammar->addSemantic("@action");
    auto sem = std::make_unique<RESemantic>(grammar.get(), semId);
    
    EXPECT_EQ(sem->nameID(), "@action");
    EXPECT_EQ(sem->toString(EmptyMask(), false), "@action");
    
    auto copy = sem->copy();
    EXPECT_EQ(copy->toString(EmptyMask(), false), "@action");
}