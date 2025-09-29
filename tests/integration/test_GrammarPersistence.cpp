#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <fstream>

using namespace syngt;

TEST(GrammarPersistenceTest, SaveAndLoad) {
    Grammar grammar1;
    grammar1.fillNew();
    
    grammar1.addNonTerminal("expr");
    
    grammar1.setNTRule("S", "'begin' , 'end'.");
    grammar1.setNTRule("expr", "term , ('+' ; '-') , term.");
    
    std::string filename = "test_grammar.grm";
    grammar1.save(filename);
    
    Grammar grammar2;
    grammar2.load(filename);
    
    EXPECT_TRUE(grammar2.hasRule("S"));
    EXPECT_TRUE(grammar2.hasRule("expr"));
    
    std::remove(filename.c_str());
}

TEST(GrammarIntegrationTest, LoadRegularizeSave) {
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
    
    size_t originalNTs = grammar.getNonTerminals().size();
    
    EXPECT_NO_THROW(grammar.regularize());
    
    std::string outFilename = "LANG_regularized.grm";
    EXPECT_NO_THROW(grammar.save(outFilename));
    
    Grammar grammar2;
    EXPECT_NO_THROW(grammar2.load(outFilename));
    
    EXPECT_GT(grammar2.getNonTerminals().size(), 1);

    int withRules = 0;
    for (const auto& nt : grammar2.getNonTerminals()) {
        if (grammar2.getNTItem(nt)->hasRoot()) {
            withRules++;
        }
    }
    EXPECT_GT(withRules, 10);
    
    std::remove(outFilename.c_str());
}