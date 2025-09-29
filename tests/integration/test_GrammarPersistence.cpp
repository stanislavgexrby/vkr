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
    
    std::cout << "Before save:\n";
    std::cout << "  S has rule: " << grammar1.hasRule("S") << "\n";
    std::cout << "  expr has rule: " << grammar1.hasRule("expr") << "\n";
    
    std::string filename = "test_grammar.grm";
    grammar1.save(filename);
    
    std::ifstream check(filename);
    std::string line;
    std::cout << "\nSaved file contents:\n";
    while (std::getline(check, line)) {
        std::cout << "  " << line << "\n";
    }
    check.close();
    
    Grammar grammar2;
    grammar2.load(filename);
    
    std::cout << "\nAfter load:\n";
    std::cout << "  S has rule: " << grammar2.hasRule("S") << "\n";
    std::cout << "  expr has rule: " << grammar2.hasRule("expr") << "\n";
    
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
    
    // Регуляризуем
    EXPECT_NO_THROW(grammar.regularize());
    
    // После регуляризации может добавиться нетерминалов (_rec версии)
    size_t newNTs = grammar.getNonTerminals().size();
    EXPECT_GE(newNTs, originalNTs);
    
    // Сохраняем регуляризованную версию
    std::string outFilename = "LANG_regularized.grm";
    EXPECT_NO_THROW(grammar.save(outFilename));
    
    // Загружаем обратно и проверяем
    Grammar grammar2;
    EXPECT_NO_THROW(grammar2.load(outFilename));
    
    EXPECT_EQ(grammar2.getNonTerminals().size(), newNTs);
    
    std::remove(outFilename.c_str());
}