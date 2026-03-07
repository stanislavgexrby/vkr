#include <gtest/gtest.h>
#include <syngt/utils/UndoRedo.h>

using namespace syngt;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void addSimpleState(UndoRedo& ur,
                           const std::vector<std::string>& names,
                           const std::vector<std::string>& values,
                           int activeIndex = 0)
{
    std::vector<bool> macroFlags(names.size(), false);
    SelectionMask selection;
    ur.addState(names, values, macroFlags, activeIndex, selection);
}

// ---------------------------------------------------------------------------
// Basic API
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, InitiallyEmpty) {
    UndoRedo ur;
    EXPECT_FALSE(ur.canUndo());
    EXPECT_FALSE(ur.canRedo());
}

TEST(UndoRedoTest, CanUndoAfterTwoStates) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});
    // Only one state — can't undo
    EXPECT_FALSE(ur.canUndo());

    addSimpleState(ur, {"A"}, {"'b'."});
    // Two distinct states — can undo
    EXPECT_TRUE(ur.canUndo());
}

TEST(UndoRedoTest, CanRedoAfterUndo) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});
    addSimpleState(ur, {"A"}, {"'b'."});

    EXPECT_FALSE(ur.canRedo());

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;
    ur.stepBack(names, values, macros, idx, sel);

    EXPECT_TRUE(ur.canRedo());
}

// ---------------------------------------------------------------------------
// Undo
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, UndoRestoresPreviousState) {
    UndoRedo ur;
    addSimpleState(ur, {"E"}, {"'a'."}, 0);
    addSimpleState(ur, {"E"}, {"'b'."}, 0);

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    bool ok = ur.stepBack(names, values, macros, idx, sel);
    EXPECT_TRUE(ok);
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "E");
    EXPECT_EQ(values[0], "'a'.");
}

TEST(UndoRedoTest, UndoReturnsFalseAtBeginning) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    bool ok = ur.stepBack(names, values, macros, idx, sel);
    EXPECT_FALSE(ok);
}

TEST(UndoRedoTest, MultipleUndoSteps) {
    UndoRedo ur;
    addSimpleState(ur, {"S"}, {"'a'."});
    addSimpleState(ur, {"S"}, {"'b'."});
    addSimpleState(ur, {"S"}, {"'c'."});

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    ur.stepBack(names, values, macros, idx, sel);
    EXPECT_EQ(values[0], "'b'.");

    ur.stepBack(names, values, macros, idx, sel);
    EXPECT_EQ(values[0], "'a'.");

    // Can't go back further
    EXPECT_FALSE(ur.canUndo());
}

// ---------------------------------------------------------------------------
// Redo
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, RedoRestoresNextState) {
    UndoRedo ur;
    addSimpleState(ur, {"E"}, {"'a'."});
    addSimpleState(ur, {"E"}, {"'b'."});

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    ur.stepBack(names, values, macros, idx, sel);
    EXPECT_EQ(values[0], "'a'.");

    bool ok = ur.stepForward(names, values, macros, idx, sel);
    EXPECT_TRUE(ok);
    EXPECT_EQ(values[0], "'b'.");
}

TEST(UndoRedoTest, RedoReturnsFalseAtEnd) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});
    addSimpleState(ur, {"A"}, {"'b'."});

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    bool ok = ur.stepForward(names, values, macros, idx, sel);
    EXPECT_FALSE(ok);
}

TEST(UndoRedoTest, UndoRedoRoundTrip) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'x'."});
    addSimpleState(ur, {"A"}, {"'y'."});
    addSimpleState(ur, {"A"}, {"'z'."});

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    ur.stepBack(names, values, macros, idx, sel); // → 'y'
    ur.stepBack(names, values, macros, idx, sel); // → 'x'
    ur.stepForward(names, values, macros, idx, sel); // → 'y'
    ur.stepForward(names, values, macros, idx, sel); // → 'z'

    EXPECT_EQ(values[0], "'z'.");
    EXPECT_FALSE(ur.canRedo());
}

// ---------------------------------------------------------------------------
// Deduplication: identical states are not stored twice
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, DuplicateStateIsNotAdded) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});
    addSimpleState(ur, {"A"}, {"'a'."}); // identical — should be ignored

    // Still only one state, so can't undo
    EXPECT_FALSE(ur.canUndo());
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, ClearData) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});
    addSimpleState(ur, {"A"}, {"'b'."});
    EXPECT_TRUE(ur.canUndo());

    ur.clearData();
    EXPECT_FALSE(ur.canUndo());
    EXPECT_FALSE(ur.canRedo());
}

// ---------------------------------------------------------------------------
// New action after undo clears redo history
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, NewStateAfterUndoClearsRedo) {
    UndoRedo ur;
    addSimpleState(ur, {"A"}, {"'a'."});
    addSimpleState(ur, {"A"}, {"'b'."});

    std::vector<std::string> names, values;
    std::vector<bool> macros;
    int idx;
    SelectionMask sel;

    ur.stepBack(names, values, macros, idx, sel); // back to 'a'
    EXPECT_TRUE(ur.canRedo());

    // Add new state — should clear redo branch
    addSimpleState(ur, {"A"}, {"'c'."});
    EXPECT_FALSE(ur.canRedo());

    // Undo still works: 'c' → 'a'
    ur.stepBack(names, values, macros, idx, sel);
    EXPECT_EQ(values[0], "'a'.");
}

// ---------------------------------------------------------------------------
// activeIndex and macroFlags roundtrip
// ---------------------------------------------------------------------------

TEST(UndoRedoTest, ActiveIndexPreserved) {
    UndoRedo ur;
    std::vector<bool> macros = {false, false};
    SelectionMask sel;
    ur.addState({"A", "B"}, {"'a'.", "'b'."}, macros, 1, sel);
    ur.addState({"A", "B"}, {"'a'.", "'c'."}, macros, 0, sel);

    std::vector<std::string> names, values;
    std::vector<bool> outMacros;
    int idx;
    SelectionMask outSel;

    ur.stepBack(names, values, outMacros, idx, outSel);
    EXPECT_EQ(idx, 1);
}

TEST(UndoRedoTest, MacroFlagsPreserved) {
    UndoRedo ur;
    std::vector<bool> macros1 = {true, false};
    std::vector<bool> macros2 = {false, true};
    SelectionMask sel;
    ur.addState({"A", "B"}, {"'a'.", "'b'."}, macros1, 0, sel);
    ur.addState({"A", "B"}, {"'a'.", "'c'."}, macros2, 0, sel);

    std::vector<std::string> names, values;
    std::vector<bool> outMacros;
    int idx;
    SelectionMask outSel;

    ur.stepBack(names, values, outMacros, idx, outSel);
    ASSERT_EQ(outMacros.size(), 2u);
    EXPECT_TRUE(outMacros[0]);
    EXPECT_FALSE(outMacros[1]);
}
