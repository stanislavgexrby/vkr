#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/analysis/RecursionAnalyzer.h>

using namespace syngt;

class RecursionAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
    }

    // Find result for a given NT name
    static const RecursionResult* findResult(const std::vector<RecursionResult>& results,
                                              const std::string& name) {
        for (const auto& r : results) {
            if (r.name == name) return &r;
        }
        return nullptr;
    }

    std::unique_ptr<Grammar> grammar;
};

// ---------------------------------------------------------------------------
// No recursion
// ---------------------------------------------------------------------------

TEST_F(RecursionAnalyzerTest, NoRecursion) {
    // S : 'a' , 'b'.
    grammar->addNonTerminal("S");
    grammar->setNTRule("S", "'a' , 'b'.");

    auto results = RecursionAnalyzer::analyze(grammar.get());
    const RecursionResult* r = findResult(results, "S");
    ASSERT_NE(r, nullptr);

    EXPECT_TRUE(r->leftRecursion.empty());
    EXPECT_TRUE(r->rightRecursion.empty());
    EXPECT_TRUE(r->anyRecursion.empty());
}

// ---------------------------------------------------------------------------
// Direct left recursion
// ---------------------------------------------------------------------------

TEST_F(RecursionAnalyzerTest, DirectLeftRecursion) {
    // E : E , '+' , term ; term.
    grammar->addNonTerminal("E");
    grammar->setNTRule("E", "E , '+' , term ; term.");

    auto results = RecursionAnalyzer::analyze(grammar.get());
    const RecursionResult* r = findResult(results, "E");
    ASSERT_NE(r, nullptr);

    EXPECT_EQ(r->leftRecursion, "direct");
    EXPECT_TRUE(r->rightRecursion.empty());
}

// ---------------------------------------------------------------------------
// Direct right recursion
// ---------------------------------------------------------------------------

TEST_F(RecursionAnalyzerTest, DirectRightRecursion) {
    // A : 'a' , A ; 'b'.
    grammar->addNonTerminal("A");
    grammar->setNTRule("A", "'a' , A ; 'b'.");

    auto results = RecursionAnalyzer::analyze(grammar.get());
    const RecursionResult* r = findResult(results, "A");
    ASSERT_NE(r, nullptr);

    EXPECT_EQ(r->rightRecursion, "direct");
    EXPECT_TRUE(r->leftRecursion.empty());
}

// ---------------------------------------------------------------------------
// Multiple NTs — one recursive, one not
// ---------------------------------------------------------------------------

TEST_F(RecursionAnalyzerTest, MixedGrammar) {
    grammar->addNonTerminal("E");
    grammar->addNonTerminal("F");
    grammar->setNTRule("E", "E , '+' , F ; F."); // left recursive
    grammar->setNTRule("F", "'id'.");              // no recursion

    auto results = RecursionAnalyzer::analyze(grammar.get());

    const RecursionResult* re = findResult(results, "E");
    const RecursionResult* rf = findResult(results, "F");
    ASSERT_NE(re, nullptr);
    ASSERT_NE(rf, nullptr);

    EXPECT_EQ(re->leftRecursion, "direct");
    EXPECT_TRUE(rf->leftRecursion.empty());
    EXPECT_TRUE(rf->rightRecursion.empty());
}

// ---------------------------------------------------------------------------
// Analyze returns entry for every NT
// ---------------------------------------------------------------------------

TEST_F(RecursionAnalyzerTest, ResultCountMatchesNTs) {
    grammar->addNonTerminal("A");
    grammar->addNonTerminal("B");
    grammar->addNonTerminal("C");
    grammar->setNTRule("A", "'a'.");
    grammar->setNTRule("B", "'b'.");
    grammar->setNTRule("C", "'c'.");

    auto results = RecursionAnalyzer::analyze(grammar.get());
    // One result per NT
    EXPECT_GE(results.size(), 3u);
}

// ---------------------------------------------------------------------------
// Empty grammar doesn't crash
// ---------------------------------------------------------------------------

TEST_F(RecursionAnalyzerTest, EmptyGrammarDoesNotCrash) {
    EXPECT_NO_THROW(RecursionAnalyzer::analyze(grammar.get()));
}
