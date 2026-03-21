#include <gtest/gtest.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/core/Types.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/utils/Creator.h>

using namespace syngt;
using namespace syngt::graphics;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a grammar with:
//   A : B ; 'x' .
//   B : 'y' .
// and B marked as a macro.
static std::unique_ptr<Grammar> makeMacroGrammar() {
    auto g = std::make_unique<Grammar>();
    g->addNonTerminal("A");
    g->addNonTerminal("B");
    g->setNTRule("A", "B ; 'x'.");
    g->setNTRule("B", "'y'.");
    g->getNTItem("B")->setMacro(true);
    return g;
}

// ---------------------------------------------------------------------------
// openMacroRefs — defaultOpen = true
// ---------------------------------------------------------------------------

TEST(MacroTest, OpenMacroRefsDefaultTrue) {
    auto g = makeMacroGrammar();

    // After openMacroRefs, the RENonTerminal node for B inside A's rule
    // should have m_isOpen == true.
    g->openMacroRefs("A", true);

    // Rebuild the diagram and check the draw-object type.
    // When B is open, drawObjectsToRight renders B's *content*, not a box —
    // so no DrawObjectMacro should appear in A's diagram.
    auto list = std::make_unique<DrawObjectList>(g.get());
    Creator::createDrawObjects(list.get(), g->getNTItem("A")->root());

    bool hasMacroBox = false;
    for (int i = 0; i < list->count(); ++i) {
        if ((*list)[i]->getType() == ctDrawObjectMacro)
            hasMacroBox = true;
    }
    EXPECT_FALSE(hasMacroBox) << "Opened macro should be inlined, not shown as a box";
}

// ---------------------------------------------------------------------------
// openMacroRefs — defaultOpen = false  (closed)
// ---------------------------------------------------------------------------

TEST(MacroTest, OpenMacroRefsDefaultFalse) {
    auto g = makeMacroGrammar();

    g->openMacroRefs("A", false);

    auto list = std::make_unique<DrawObjectList>(g.get());
    Creator::createDrawObjects(list.get(), g->getNTItem("A")->root());

    bool hasMacroBox = false;
    for (int i = 0; i < list->count(); ++i) {
        if ((*list)[i]->getType() == ctDrawObjectMacro)
            hasMacroBox = true;
    }
    EXPECT_TRUE(hasMacroBox) << "Closed macro should produce a DrawObjectMacro box";
}

// ---------------------------------------------------------------------------
// DrawObjectMacro created for closed macro reference
// ---------------------------------------------------------------------------

TEST(MacroTest, DrawObjectTypeForMacro) {
    auto g = makeMacroGrammar();
    // B is a macro but NOT opened — diagram of A should contain DrawObjectMacro

    auto list = std::make_unique<DrawObjectList>(g.get());
    Creator::createDrawObjects(list.get(), g->getNTItem("A")->root());

    bool hasMacroBox = false;
    bool hasNonTerminalBox = false;
    for (int i = 0; i < list->count(); ++i) {
        int t = (*list)[i]->getType();
        if (t == ctDrawObjectMacro) hasMacroBox = true;
        if (t == ctDrawObjectNonTerminal) hasNonTerminalBox = true;
    }
    EXPECT_TRUE(hasMacroBox) << "Macro reference should create DrawObjectMacro";
    // B is a macro, so it must NOT produce a plain NonTerminal box
    EXPECT_FALSE(hasNonTerminalBox) << "Macro reference must not create DrawObjectNonTerminal";
}

// ---------------------------------------------------------------------------
// Non-macro NT still produces DrawObjectNonTerminal
// ---------------------------------------------------------------------------

TEST(MacroTest, DrawObjectTypeForNonMacro) {
    auto g = std::make_unique<Grammar>();
    g->addNonTerminal("A");
    g->addNonTerminal("B");
    g->setNTRule("A", "B ; 'x'.");
    g->setNTRule("B", "'y'.");
    // B is NOT a macro

    auto list = std::make_unique<DrawObjectList>(g.get());
    Creator::createDrawObjects(list.get(), g->getNTItem("A")->root());

    bool hasMacroBox = false;
    bool hasNonTerminalBox = false;
    for (int i = 0; i < list->count(); ++i) {
        int t = (*list)[i]->getType();
        if (t == ctDrawObjectMacro) hasMacroBox = true;
        if (t == ctDrawObjectNonTerminal) hasNonTerminalBox = true;
    }
    EXPECT_FALSE(hasMacroBox);
    EXPECT_TRUE(hasNonTerminalBox);
}

// ---------------------------------------------------------------------------
// allMacroWasOpened — normal case
// ---------------------------------------------------------------------------

TEST(MacroTest, AllMacroWasOpenedNormal) {
    auto g = makeMacroGrammar();
    g->openMacroRefs("A", true);

    NTListItem* itemA = g->getNTItem("A");
    ASSERT_NE(itemA, nullptr);
    ASSERT_NE(itemA->root(), nullptr);

    EXPECT_TRUE(itemA->root()->allMacroWasOpened());
}

// ---------------------------------------------------------------------------
// allMacroWasOpened — circular reference throws
// ---------------------------------------------------------------------------

TEST(MacroTest, AllMacroWasOpenedCircularThrows) {
    // A : B .   B : A .   both are macros → circular reference
    auto g = std::make_unique<Grammar>();
    g->addNonTerminal("A");
    g->addNonTerminal("B");
    g->setNTRule("A", "B.");
    g->setNTRule("B", "A.");
    g->getNTItem("A")->setMacro(true);
    g->getNTItem("B")->setMacro(true);

    g->openMacroRefs("A", true);

    NTListItem* itemA = g->getNTItem("A");
    ASSERT_NE(itemA, nullptr);
    ASSERT_NE(itemA->root(), nullptr);

    EXPECT_THROW(itemA->root()->allMacroWasOpened(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Grammar save/load preserves macro status
// ---------------------------------------------------------------------------

TEST(MacroTest, SaveLoadPreservesMacroFlag) {
    auto g = makeMacroGrammar();

    std::string filename = "test_macro.grm";
    g->save(filename);

    Grammar g2;
    g2.load(filename);

    NTListItem* itemB = g2.getNTItem("B");
    ASSERT_NE(itemB, nullptr);
    EXPECT_TRUE(itemB->isMacro()) << "Macro flag for B must survive save/load";

    NTListItem* itemA = g2.getNTItem("A");
    ASSERT_NE(itemA, nullptr);
    EXPECT_FALSE(itemA->isMacro()) << "A must remain a non-macro after save/load";

    std::remove(filename.c_str());
}

// ---------------------------------------------------------------------------
// openMacroRefs on NT with no macros is a no-op
// ---------------------------------------------------------------------------

TEST(MacroTest, OpenMacroRefsNoMacrosIsNoop) {
    auto g = std::make_unique<Grammar>();
    g->addNonTerminal("A");
    g->setNTRule("A", "'x' ; 'y'.");

    EXPECT_NO_THROW(g->openMacroRefs("A", true));

    auto list = std::make_unique<DrawObjectList>(g.get());
    Creator::createDrawObjects(list.get(), g->getNTItem("A")->root());

    for (int i = 0; i < list->count(); ++i) {
        EXPECT_NE((*list)[i]->getType(), ctDrawObjectMacro);
    }
}
