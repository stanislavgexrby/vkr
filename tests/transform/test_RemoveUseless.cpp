#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/transform/RemoveUseless.h>

using namespace syngt;

class RemoveUselessTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }
    
    std::unique_ptr<Grammar> grammar;
};

TEST_F(RemoveUselessTest, RemoveUnreachable) {
    // S → A
    // A → 'a'
    // B → 'b'
    
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    
    grammar->setNTRule("S", "A.");
    grammar->setNTRule("A", "'a'.");
    grammar->setNTRule("B", "'b'.");
    
    RemoveUseless::remove(grammar.get());
    
    NTListItem* b = grammar->getNTItem("B");
    if (b) {
        EXPECT_FALSE(b->hasRoot());
    }
}

TEST_F(RemoveUselessTest, RemoveNonProductive) {
    // S → A
    // A → B
    
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A.");
    grammar->setNTRule("A", "B.");
    
    RemoveUseless::remove(grammar.get());
    
    NTListItem* a = grammar->getNTItem("A");
    if (a) {
        EXPECT_FALSE(a->hasRoot());
    }
}

TEST_F(RemoveUselessTest, KeepProductive) {
    // S → A
    // A → 'a'
    
    grammar->addNonTerminal("A");
    grammar->setNTRule("S", "A.");
    grammar->setNTRule("A", "'a'.");
    
    RemoveUseless::remove(grammar.get());
    
    NTListItem* s = grammar->getNTItem("S");
    NTListItem* a = grammar->getNTItem("A");
    
    EXPECT_TRUE(s->hasRoot());
    EXPECT_TRUE(a->hasRoot());
}

TEST_F(RemoveUselessTest, ComplexGrammar) {
    // S → A B
    // A → 'a'
    // B → 'b'
    // C → 'c'
    // D → E
    
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    grammar->addNonTerminal("D");
    
    grammar->setNTRule("S", "A , B.");
    grammar->setNTRule("A", "'a'.");
    grammar->setNTRule("B", "'b'.");
    grammar->setNTRule("C", "'c'.");
    grammar->setNTRule("D", "E.");
    
    RemoveUseless::remove(grammar.get());
    
    EXPECT_TRUE(grammar->getNTItem("S")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("A")->hasRoot());
    EXPECT_TRUE(grammar->getNTItem("B")->hasRoot());
    
    NTListItem* c = grammar->getNTItem("C");
    NTListItem* d = grammar->getNTItem("D");
    if (c) EXPECT_FALSE(c->hasRoot());
    if (d) EXPECT_FALSE(d->hasRoot());
}