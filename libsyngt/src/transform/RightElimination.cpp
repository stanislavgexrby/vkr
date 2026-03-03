#include <syngt/transform/RightElimination.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/RETerminal.h>
#include <memory>

namespace syngt {

// ---------------------------------------------------------------------------
// Internal data structure
// ---------------------------------------------------------------------------

struct RightTransformation {
    std::unique_ptr<RETree> RA;  // coefficient of A (what precedes A)
    std::unique_ptr<RETree> RB;  // non-recursive part
    bool E = false;              // can this expression produce epsilon?

    RightTransformation() = default;
    RightTransformation(std::unique_ptr<RETree> ra, std::unique_ptr<RETree> rb, bool e)
        : RA(std::move(ra)), RB(std::move(rb)), E(e) {}
};

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------

static bool isEpsilonNode(const RETree* node, Grammar* grammar) {
    if (!node) return false;
    if (auto sem = dynamic_cast<const RESemantic*>(node)) {
        return grammar->getSemanticName(sem->id()) == "@";
    }
    if (auto term = dynamic_cast<const RETerminal*>(node)) {
        return term->getID() == 0;
    }
    return false;
}

static std::unique_ptr<RETree> makeEpsilon(Grammar* grammar) {
    return std::make_unique<RESemantic>(grammar, grammar->addSemantic("@"));
}

// nullptr | X = X,  X | nullptr = X,  X | Y = REOr(X,Y)
static std::unique_ptr<RETree> createOr(std::unique_ptr<RETree> a, std::unique_ptr<RETree> b) {
    if (!a) return b;
    if (!b) return a;
    return REOr::make(std::move(a), std::move(b));
}

// nullptr, X = nullptr,  X, nullptr = nullptr,  X, Y = REAnd(X,Y)
static std::unique_ptr<RETree> createAnd(std::unique_ptr<RETree> a, std::unique_ptr<RETree> b) {
    if (!a || !b) return nullptr;
    return REAnd::make(std::move(a), std::move(b));
}

// nullptr, X = X,  X, nullptr = X,  X, Y = REAnd(X,Y)
static std::unique_ptr<RETree> createAndAlt(std::unique_ptr<RETree> a, std::unique_ptr<RETree> b) {
    if (!a) return b;
    if (!b) return a;
    return REAnd::make(std::move(a), std::move(b));
}

// epsilon | tree
static std::unique_ptr<RETree> createOrEmpty(std::unique_ptr<RETree> tree, Grammar* grammar) {
    return REOr::make(makeEpsilon(grammar), std::move(tree));
}

// epsilon * tree  (if tree is nullptr, returns epsilon)
static std::unique_ptr<RETree> createUnaryIteration(std::unique_ptr<RETree> tree, Grammar* grammar) {
    if (!tree) return makeEpsilon(grammar);
    return REIteration::make(makeEpsilon(grammar), std::move(tree));
}

// ---------------------------------------------------------------------------
// Core recursive decomposition
// ---------------------------------------------------------------------------

static RightTransformation computeRightEl(const RETree* node,
                                          const NTListItem* nt,
                                          Grammar* grammar);

static RightTransformation computeRightEl(const RETree* node,
                                          const NTListItem* nt,
                                          Grammar* grammar) {
    if (!node) return {};

    const int ntId = grammar->findNonTerminal(nt->name());

    // --- RENonTerminal ---
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->getID() == ntId) {
            // This IS A: T = ε · A | ∅
            return { makeEpsilon(grammar), nullptr, false };
        } else {
            // Some other nonterminal B: T = ∅ · A | B
            return { nullptr, node->copy(), false };
        }
    }

    // --- RETerminal ---
    if (auto termNode = dynamic_cast<const RETerminal*>(node)) {
        if (termNode->getID() == 0) {
            // Empty terminal: E = true
            return { nullptr, nullptr, true };
        }
        return { nullptr, node->copy(), false };
    }

    // --- RESemantic ---
    if (auto semNode = dynamic_cast<const RESemantic*>(node)) {
        if (grammar->getSemanticName(semNode->id()) == "@") {
            // Epsilon semantic: E = true
            return { nullptr, nullptr, true };
        }
        return { nullptr, node->copy(), false };
    }

    // --- REOr(L, R) ---
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        auto LTr = computeRightEl(orNode->left(), nt, grammar);
        auto RTr = computeRightEl(orNode->right(), nt, grammar);
        return {
            createOr(std::move(LTr.RA), std::move(RTr.RA)),
            createOr(std::move(LTr.RB), std::move(RTr.RB)),
            LTr.E || RTr.E
        };
    }

    // --- REAnd(L, R) ---
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        const RETree* L = andNode->left();
        const RETree* R = andNode->right();
        if (!L || !R) return { nullptr, node->copy(), false };

        auto RTr = computeRightEl(R, nt, grammar);

        if (!RTr.E) {
            // R cannot produce epsilon: L·R = L·RTr.RA·A | L·RTr.RB
            return {
                createAnd(L->copy(), std::move(RTr.RA)),
                createAnd(L->copy(), std::move(RTr.RB)),
                false
            };
        } else {
            // R can produce epsilon: also consider right-recursion from L itself
            auto LTr = computeRightEl(L, nt, grammar);
            return {
                createOr(createAnd(L->copy(), std::move(RTr.RA)), std::move(LTr.RA)),
                createOr(createAnd(L->copy(), std::move(RTr.RB)), std::move(LTr.RB)),
                LTr.E
            };
        }
    }

    // --- REIteration(L, R) ---
    if (auto iterNode = dynamic_cast<const REIteration*>(node)) {
        const RETree* L = iterNode->left();
        const RETree* R = iterNode->right();
        if (!L || !R) return { nullptr, node->copy(), false };

        if (isEpsilonNode(L, grammar)) {
            // Standard unary star: ε*R = R*
            // (ε*R) = (ε*R)·RTr.RA·A | (ε*R)·RTr.RB
            auto RTr = computeRightEl(R, nt, grammar);
            return {
                createAnd(node->copy(), std::move(RTr.RA)),
                createAnd(node->copy(), std::move(RTr.RB)),
                true
            };
        } else {
            // General L*R = L·(R·L)* — reduce and recurse
            // Build: REAnd(L, REIteration(ε, REAnd(R, L)))
            auto innerAnd = REAnd::make(R->copy(), L->copy());
            auto iterEps  = createUnaryIteration(std::move(innerAnd), grammar);
            auto temp     = REAnd::make(L->copy(), std::move(iterEps));
            return computeRightEl(temp.get(), nt, grammar);
        }
    }

    // Unknown node type: treat as non-recursive leaf
    return { nullptr, node->copy(), false };
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool RightElimination::hasDirectRightRecursion(NTListItem* nt, Grammar* grammar) {
    if (!nt || !grammar) return false;
    RETree* root = nt->root();
    if (!root) return false;
    auto tr = computeRightEl(root, nt, grammar);
    return tr.RA != nullptr;
}

void RightElimination::eliminateForNonTerminal(NTListItem* nt, Grammar* grammar) {
    if (!nt || !grammar) return;
    if (!hasDirectRightRecursion(nt, grammar)) return;

    RETree* root = nt->root();
    if (!root) return;

    auto tr = computeRightEl(root, nt, grammar);

    const bool raWasNonNull = (tr.RA != nullptr);
    const bool rbWasNonNull = (tr.RB != nullptr);

    // Build: RB , RA*
    auto raIter  = createUnaryIteration(std::move(tr.RA), grammar);
    auto newRoot = createAndAlt(std::move(tr.RB), std::move(raIter));

    // If original could produce epsilon but both parts were present, add explicit ε
    if (tr.E && raWasNonNull && rbWasNonNull) {
        newRoot = createOrEmpty(std::move(newRoot), grammar);
    }

    if (newRoot) {
        nt->setRoot(std::move(newRoot));
    }
}

void RightElimination::eliminate(Grammar* grammar) {
    if (!grammar) return;

    auto nts = grammar->getNonTerminals();
    for (size_t i = 0; i < nts.size(); ++i) {
        NTListItem* nt = grammar->getNTItemByIndex(static_cast<int>(i));
        if (nt) {
            eliminateForNonTerminal(nt, grammar);
        }
    }
}

} // namespace syngt
