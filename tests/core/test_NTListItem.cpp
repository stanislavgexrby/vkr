#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/REAnd.h>

using namespace syngt;

TEST(NTListItemTest, SetValueParsesToTree) {
    Grammar grammar;
    NTListItem item(&grammar, "test");
    
    item.setValue("'begin' , 'end'.");
    
    EXPECT_TRUE(item.hasRoot());
    EXPECT_EQ(item.value(), "'begin' , 'end'.");
}

TEST(NTListItemTest, SetRootUpdatesValue) {
    Grammar grammar;
    int id1 = grammar.addTerminal("begin");
    int id2 = grammar.addTerminal("end");
    
    NTListItem item(&grammar, "test");
    
    auto term1 = std::make_unique<RETerminal>(&grammar, id1);
    auto term2 = std::make_unique<RETerminal>(&grammar, id2);
    auto tree = REAnd::make(std::move(term1), std::move(term2));
    
    item.setRoot(std::move(tree));
    
    EXPECT_TRUE(item.hasRoot());
    EXPECT_EQ(item.value(), "'begin','end'");
}

TEST(NTListItemTest, CopyRETree) {
    Grammar grammar;
    NTListItem item(&grammar, "test");
    
    item.setValue("'test'.");
    
    auto copy = item.copyRETree();
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->toString(EmptyMask(), false), "'test'");
}