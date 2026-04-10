#include <syngt/analysis/Minimize.h>
#include <syngt/analysis/Minimization.h>
#include <syngt/analysis/DFAToREGEX.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/RENonTerminal.h>

namespace syngt {

// ---------------------------------------------------------------------------
// buildMinimizationTable — external recursive visitor
// (analog of TRE_Tree.buildMinimizationTable virtual method from Pascal)
//
// Builds an NFA in 'table' representing the language of 'node',
// with transitions from rec.start to rec.finish.
// ---------------------------------------------------------------------------

static void buildMinimizationTable(const RETree* node,
                                    MinimizationTable& table,
                                    MinRecord rec,
                                    Grammar* grammar) {
    if (!node) return;

    // --- REOr(L, R): both alternatives share the same start/finish ---
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        buildMinimizationTable(orNode->left(),  table, rec, grammar);
        buildMinimizationTable(orNode->right(), table, rec, grammar);
        return;
    }

    // --- REAnd(L, R): sequential — introduce intermediate state ---
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        State intermediate = table.createState();
        buildMinimizationTable(andNode->left(),  table, MinRecord{rec.start, intermediate}, grammar);
        buildMinimizationTable(andNode->right(), table, MinRecord{intermediate, rec.finish}, grammar);
        return;
    }

    // --- REIteration(L, R): L#R = L(RL)* ---
    // Pascal algorithm:
    //   newState → finish (epsilon)
    //   L: start → newState
    //   R: newState → start  (loop back)
    if (auto iterNode = dynamic_cast<const REIteration*>(node)) {
        const RETree* L = iterNode->left();
        const RETree* R = iterNode->right();
        if (!L || !R) return;

        State savedFinish = rec.finish;
        State newState    = table.createState();

        // epsilon transition: newState → savedFinish
        table.linkStates(newState, savedFinish, "\"\"");

        // Left operand: rec.start → newState
        buildMinimizationTable(L, table, MinRecord{rec.start, newState}, grammar);

        // Right operand: newState → rec.start (reversed — loop back)
        buildMinimizationTable(R, table, MinRecord{newState, rec.start}, grammar);
        return;
    }

    // --- RETerminal: single transition on the terminal symbol ---
    if (auto termNode = dynamic_cast<const RETerminal*>(node)) {
        // TerminalList::getString(0) returns "@" when m_items[0]="" — but we must
        // store epsilon as "\"\"" in the table so fromMinimizationTable recognises it.
        int epsilonId = grammar->findTerminal("");
        bool isEpsilon = (epsilonId >= 0 && termNode->getID() == epsilonId);
        std::string sym = isEpsilon
            ? "\"\""
            : "\"" + grammar->getTerminalName(termNode->getID()) + "\"";
        table.linkStates(rec.start, rec.finish, sym);
        return;
    }

    // --- RESemantic: epsilon ("@") or named semantic action ---
    if (auto semNode = dynamic_cast<const RESemantic*>(node)) {
        std::string name = grammar->getSemanticName(semNode->id());
        if (name == "@") {
            // epsilon — same as empty terminal
            table.linkStates(rec.start, rec.finish, "\"\"");
        } else {
            // Store with $ prefix so DFAToRegex recognises it as a semantic.
            // name already contains '$' (e.g. "$add"), store as-is.
            table.linkStates(rec.start, rec.finish, name);
        }
        return;
    }

    // --- RENonTerminal: single transition on the nonterminal name ---
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        std::string sym = grammar->getNonTerminalName(ntNode->getID());
        table.linkStates(rec.start, rec.finish, sym);
        return;
    }

    // Unknown node: treat as epsilon
    table.linkStates(rec.start, rec.finish, "\"\"");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Minimize::minimize(Grammar* grammar) {
    if (!grammar) return;

    int count = static_cast<int>(grammar->getNonTerminals().size());

    for (int i = 0; i < count; ++i) {
        NTListItem* nt = grammar->getNTItemByIndex(i);
        if (!nt || !nt->root()) continue;

        // 1. Build NFA from the RE tree
        MinimizationTable table;
        MinRecord rec;  // {start: StartState=0, finish: FinalState=1}
        buildMinimizationTable(nt->root(), table, rec, grammar);

        // 2. Minimize (merge equivalent states)
        table.minimize();

        // 3. Convert minimized NFA back to RE using state elimination (Arden's method)
        auto converter = DFAToRegex::fromMinimizationTable(grammar, &table);
        converter->removeAllStates();

        // 4. Set the resulting RE as the new rule
        auto result = converter->getRegularExpression();
        if (result) {
            nt->setRoot(std::move(result));
        }
    }
}

} // namespace syngt
