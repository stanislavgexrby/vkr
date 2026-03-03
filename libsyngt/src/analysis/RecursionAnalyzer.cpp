#include <syngt/analysis/RecursionAnalyzer.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/RENonTerminal.h>

#include <set>
#include <string>
#include <vector>

namespace syngt {

// ---------------------------------------------------------------------------
// Internal helpers — port of TAnalyzeForm helpers from Analyzer.pas
// ---------------------------------------------------------------------------

// Check if the RE subtree can produce the empty string (epsilon).
// Works on direct syntax; RENonTerminal is treated as non-epsilon
// (same conservative approximation as the Pascal string-level check).
static bool canProduceEpsilon(const RETree* node, const Grammar* grammar) {
    if (!node) return false;

    if (auto* orNode = dynamic_cast<const REOr*>(node))
        return canProduceEpsilon(orNode->left(),  grammar) ||
               canProduceEpsilon(orNode->right(), grammar);

    if (auto* andNode = dynamic_cast<const REAnd*>(node))
        return canProduceEpsilon(andNode->left(),  grammar) &&
               canProduceEpsilon(andNode->right(), grammar);

    // A#B = A(BA)*: produces epsilon iff A itself produces epsilon
    if (auto* iterNode = dynamic_cast<const REIteration*>(node))
        return canProduceEpsilon(iterNode->left(), grammar);

    // Empty terminal (id=0, name="") is epsilon
    if (auto* termNode = dynamic_cast<const RETerminal*>(node))
        return grammar->getTerminalName(termNode->getID()).empty();

    // Semantic "@" is used as an epsilon marker
    if (auto* semNode = dynamic_cast<const RESemantic*>(node))
        return grammar->getSemanticName(semNode->id()) == "@";

    return false;
}

// Collect all NT names referenced anywhere in the subtree (port of GetArray on 'full' form).
static void collectFull(const RETree* node, const Grammar* grammar,
                        std::set<std::string>& out) {
    if (!node) return;

    if (auto* nt = dynamic_cast<const RENonTerminal*>(node)) {
        out.insert(grammar->getNonTerminalName(nt->getID()));
        return;
    }
    // Recurse into both children for binary ops (Or/And/Iteration)
    collectFull(node->left(),  grammar, out);
    collectFull(node->right(), grammar, out);
}

// Collect NT names that can appear in leftmost position
// (port of TrimForLeft + GetArray logic).
//
// REAnd(L,R) : leftRefs(L); if canEpsilon(L) also leftRefs(R)
// REIteration(L,R) = L(RL)*: same as REAnd for left position
static void collectLeft(const RETree* node, const Grammar* grammar,
                        std::set<std::string>& out) {
    if (!node) return;

    if (auto* nt = dynamic_cast<const RENonTerminal*>(node)) {
        out.insert(grammar->getNonTerminalName(nt->getID()));
        return;
    }
    if (auto* orNode = dynamic_cast<const REOr*>(node)) {
        collectLeft(orNode->left(),  grammar, out);
        collectLeft(orNode->right(), grammar, out);
        return;
    }
    if (auto* andNode = dynamic_cast<const REAnd*>(node)) {
        collectLeft(andNode->left(), grammar, out);
        if (canProduceEpsilon(andNode->left(), grammar))
            collectLeft(andNode->right(), grammar, out);
        return;
    }
    // A#B = A(BA)*: leftmost is A; if A can be ε, B could come first
    if (auto* iterNode = dynamic_cast<const REIteration*>(node)) {
        collectLeft(iterNode->left(), grammar, out);
        if (canProduceEpsilon(iterNode->left(), grammar))
            collectLeft(iterNode->right(), grammar, out);
        return;
    }
    // Terminals / semantics contribute nothing to NT sets
}

// Collect NT names that can appear in rightmost position
// (port of TrimForRight + GetArray logic).
//
// REAnd(L,R) : rightRefs(R); if canEpsilon(R) also rightRefs(L)
// REIteration(L,R) = L(RL)*: last element is always L; if L=ε, R can be last
static void collectRight(const RETree* node, const Grammar* grammar,
                         std::set<std::string>& out) {
    if (!node) return;

    if (auto* nt = dynamic_cast<const RENonTerminal*>(node)) {
        out.insert(grammar->getNonTerminalName(nt->getID()));
        return;
    }
    if (auto* orNode = dynamic_cast<const REOr*>(node)) {
        collectRight(orNode->left(),  grammar, out);
        collectRight(orNode->right(), grammar, out);
        return;
    }
    if (auto* andNode = dynamic_cast<const REAnd*>(node)) {
        collectRight(andNode->right(), grammar, out);
        if (canProduceEpsilon(andNode->right(), grammar))
            collectRight(andNode->left(), grammar, out);
        return;
    }
    // A#B = A(BA)*: rightmost is always A; if A=ε, B is effectively last
    if (auto* iterNode = dynamic_cast<const REIteration*>(node)) {
        collectRight(iterNode->left(), grammar, out);
        if (canProduceEpsilon(iterNode->left(), grammar))
            collectRight(iterNode->right(), grammar, out);
        return;
    }
}

// ---------------------------------------------------------------------------
// AnalyzeItem — internal per-NT record (port of TAnalyzeItem)
// ---------------------------------------------------------------------------

struct AnalyzeItem {
    std::string          name;
    std::set<std::string> full;   // all NT refs
    std::set<std::string> left;   // left-position NT refs
    std::set<std::string> right;  // right-position NT refs
};

// ---------------------------------------------------------------------------
// analyzeOnePart — port of TAnalyzeForm.AnalyzeOnePart
//
// mode: 1=left, 3=right, else=full
// Returns "direct", "indirect", or "".
// ---------------------------------------------------------------------------
static std::string analyzeOnePart(const std::vector<AnalyzeItem>& items,
                                   size_t ind, int mode) {
    const std::string& target = items[ind].name;

    auto getSet = [&](size_t i) -> const std::set<std::string>& {
        if (mode == 1) return items[i].left;
        if (mode == 3) return items[i].right;
        return items[i].full;
    };

    // BFS from ind: follow NT references, detect reachability
    std::vector<bool> go(items.size(), false);
    std::vector<bool> done(items.size(), false);
    go[ind] = true;

    std::string found;

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < items.size(); ++i) {
            if (!go[i] || done[i]) continue;
            done[i] = true;
            changed = true;

            for (const auto& ref : getSet(i)) {
                // Any reachable NT (including ind itself) referencing target → indirect
                if (ref == target) found = "indirect";
                // Expand traversal
                for (size_t q = 0; q < items.size(); ++q) {
                    if (!go[q] && items[q].name == ref)
                        go[q] = true;
                }
            }
        }
    }

    // Direct: ind's own set contains target (overrides indirect)
    for (const auto& ref : getSet(ind)) {
        if (ref == target) { found = "direct"; break; }
    }

    return found;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::vector<RecursionResult> RecursionAnalyzer::analyze(const Grammar* grammar) {
    if (!grammar) return {};

    auto ntNames = grammar->getNonTerminals();
    int count = static_cast<int>(ntNames.size());

    // Build per-NT reference sets
    std::vector<AnalyzeItem> items(count);
    for (int i = 0; i < count; ++i) {
        items[i].name = ntNames[i];
        NTListItem* nt = grammar->getNTItemByIndex(i);
        if (nt && nt->root()) {
            collectFull (nt->root(), grammar, items[i].full);
            collectLeft (nt->root(), grammar, items[i].left);
            collectRight(nt->root(), grammar, items[i].right);
        }
    }

    // Run analysis for each NT
    std::vector<RecursionResult> results(count);
    for (int i = 0; i < count; ++i) {
        results[i].name           = items[i].name;
        results[i].leftRecursion  = analyzeOnePart(items, static_cast<size_t>(i), 1);
        results[i].anyRecursion   = analyzeOnePart(items, static_cast<size_t>(i), 2);
        results[i].rightRecursion = analyzeOnePart(items, static_cast<size_t>(i), 3);
    }

    return results;
}

} // namespace syngt
