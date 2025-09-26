#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <fstream>

using namespace syngt;

TEST(LoadGrammarTest, LoadLANGGRM) {
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
    EXPECT_EQ(nts[0], "S");
    EXPECT_EQ(nts[1], "program");
}