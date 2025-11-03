#include <gtest/gtest.h>
#include <syngt/utils/Creator.h>
#include <syngt/core/Grammar.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>

using namespace syngt;
using namespace syngt::graphics;

class CreatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
        list = std::make_unique<DrawObjectList>(grammar.get());
    }
    
    std::unique_ptr<Grammar> grammar;
    std::unique_ptr<DrawObjectList> list;
};

TEST_F(CreatorTest, CreateEmptyDiagram) {
    int termId = grammar->addTerminal("begin");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(list.get(), tree.get());
    
    EXPECT_GE(list->count(), 2);
    EXPECT_GT(list->width(), 0);
    EXPECT_GT(list->height(), 0);
}

TEST_F(CreatorTest, CreateSequence) {
    int id1 = grammar->addTerminal("begin");
    int id2 = grammar->addTerminal("end");
    
    auto term1 = std::make_unique<RETerminal>(grammar.get(), id1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), id2);
    auto seq = REAnd::make(std::move(term1), std::move(term2));
    
    Creator::createDrawObjects(list.get(), seq.get());
    
    EXPECT_GE(list->count(), 2);
    EXPECT_GT(list->width(), 40);
}

TEST_F(CreatorTest, CreateAlternative) {
    int id1 = grammar->addTerminal("begin");
    int id2 = grammar->addTerminal("end");
    
    auto term1 = std::make_unique<RETerminal>(grammar.get(), id1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), id2);
    auto alt = REOr::make(std::move(term1), std::move(term2));
    
    Creator::createDrawObjects(list.get(), alt.get());
    
    EXPECT_GE(list->count(), 2);
    EXPECT_GT(list->width(), 0);
}

TEST_F(CreatorTest, DiagramSizePositive) {
    int termId = grammar->addTerminal("test");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(list.get(), tree.get());
    
    EXPECT_GT(list->width(), 0);
    EXPECT_GT(list->height(), 0);
}

TEST_F(CreatorTest, MultipleCreations) {
    int termId = grammar->addTerminal("test");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(list.get(), tree.get());
    int count1 = list->count();
    
    Creator::createDrawObjects(list.get(), tree.get());
    int count2 = list->count();
    
    EXPECT_EQ(count1, count2);
}

TEST_F(CreatorTest, NullPointerHandling) {
    int termId = grammar->addTerminal("test");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(nullptr, tree.get());
    Creator::createDrawObjects(list.get(), nullptr);
    
    EXPECT_EQ(list->count(), 0);
}