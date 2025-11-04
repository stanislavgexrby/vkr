#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <fstream>

using namespace syngt;

TEST(GrammarTest, Initialization) {
    Grammar grammar;
    grammar.fillNew();
    
    EXPECT_EQ(grammar.getNonTerminals().size(), 0);
}

TEST(GrammarTest, AddTerminals) {
    Grammar grammar;
    
    int id1 = grammar.addTerminal("begin");
    int id2 = grammar.addTerminal("end");
    int id3 = grammar.addTerminal("begin");  // Duplicate
    
    EXPECT_EQ(id1, 0);
    EXPECT_EQ(id2, 1);
    EXPECT_EQ(id3, 0);
    EXPECT_EQ(grammar.getTerminals().size(), 2);
}

TEST(GrammarTest, FindTerminals) {
    Grammar grammar;
    
    grammar.addTerminal("begin");
    grammar.addTerminal("end");
    
    EXPECT_EQ(grammar.findTerminal("begin"), 0);
    EXPECT_EQ(grammar.findTerminal("end"), 1);
    EXPECT_EQ(grammar.findTerminal("middle"), -1);
}

TEST(GrammarTest, AddMultipleTypes) {
    Grammar grammar;
    grammar.fillNew();
    
    grammar.addTerminal("program");
    grammar.addTerminal("begin");
    grammar.addTerminal("end");
    
    grammar.addNonTerminal("statement");
    grammar.addNonTerminal("expression");
    
    grammar.addSemantic("@action1");
    
    EXPECT_GE(grammar.getTerminals().size(), 3);
    EXPECT_GE(grammar.getNonTerminals().size(), 2);
}

TEST(GrammarTest, SaveAndLoad) {
    Grammar grammar1;
    grammar1.fillNew();
    
    grammar1.addNonTerminal("start");
    grammar1.addNonTerminal("expr");
    
    grammar1.setNTRule("start", "'begin' , 'end'.");
    grammar1.setNTRule("expr", "term , ('+' ; '-') , term.");
    
    std::string filename = "test_grammar.grm";
    grammar1.save(filename);
    
    Grammar grammar2;
    grammar2.load(filename);
    
    EXPECT_TRUE(grammar2.hasRule("start"));
    EXPECT_TRUE(grammar2.hasRule("expr"));
    
    std::remove(filename.c_str());
}

TEST(GrammarTest, LoadRegularizeSave) {
    Grammar grammar;
    
    std::string filename;
    if (std::ifstream("examples/grammars/LANG.GRM").good()) {
        filename = "examples/grammars/LANG.GRM";
    } else if (std::ifstream("../examples/grammars/LANG.GRM").good()) {
        filename = "../examples/grammars/LANG.GRM";
    } else if (std::ifstream("../../examples/grammars/LANG.GRM").good()) {
        filename = "../../examples/grammars/LANG.GRM";
    } else {
        GTEST_SKIP() << "LANG.GRM not found";
    }
    
    EXPECT_NO_THROW(grammar.load(filename));
    
    EXPECT_NO_THROW(grammar.regularize());
    
    std::string outFilename = "LANG_regularized.grm";
    EXPECT_NO_THROW(grammar.save(outFilename));
    
    Grammar grammar2;
    EXPECT_NO_THROW(grammar2.load(outFilename));
    
    EXPECT_GT(grammar2.getNonTerminals().size(), 1);

    int withRules = 0;
    for (const auto& nt : grammar2.getNonTerminals()) {
        if (grammar2.getNTItem(nt) && grammar2.getNTItem(nt)->hasRoot()) {
            withRules++;
        }
    }
    EXPECT_GT(withRules, 10);
    
    std::remove(outFilename.c_str());
}

TEST(GrammarTest, LoadLANGGRM) {
    Grammar grammar;
    
    std::string filename;
    if (std::ifstream("examples/grammars/LANG.GRM").good()) {
        filename = "examples/grammars/LANG.GRM";
    } else if (std::ifstream("../examples/grammars/LANG.GRM").good()) {
        filename = "../examples/grammars/LANG.GRM";
    } else if (std::ifstream("../../examples/grammars/LANG.GRM").good()) {
        filename = "../../examples/grammars/LANG.GRM";
    } else {
        GTEST_SKIP() << "LANG.GRM not found, skipping test";
    }
    
    EXPECT_NO_THROW(grammar.load(filename));
    
    EXPECT_GT(grammar.getTerminals().size(), 50);
    EXPECT_GT(grammar.getNonTerminals().size(), 50);
    
    auto nts = grammar.getNonTerminals();
    EXPECT_EQ(nts[0], "program");
    EXPECT_EQ(nts[1], "initiations");
    
    EXPECT_TRUE(grammar.hasRule("program"));
}