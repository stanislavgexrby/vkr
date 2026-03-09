#include <syngt/transform/Regularize.h>
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
// Internal data structures
// ---------------------------------------------------------------------------

struct LeftTransformation {
    std::unique_ptr<RETree> R1;  // T = A·R1 | R2
    std::unique_ptr<RETree> R2;
    bool E = false;

    LeftTransformation() = default;
    LeftTransformation(std::unique_ptr<RETree> r1, std::unique_ptr<RETree> r2, bool e)
        : R1(std::move(r1)), R2(std::move(r2)), E(e) {}
};

struct RightTransformation {
    std::unique_ptr<RETree> RA;  // T = RA·A | RB
    std::unique_ptr<RETree> RB;
    bool E = false;

    RightTransformation() = default;
    RightTransformation(std::unique_ptr<RETree> ra, std::unique_ptr<RETree> rb, bool e)
        : RA(std::move(ra)), RB(std::move(rb)), E(e) {}
};

// ---------------------------------------------------------------------------
// File-local helpers (TransCreator.pas analogs)
// ---------------------------------------------------------------------------

static bool isEpsilonNode(const RETree* node, Grammar* grammar) {
    if (!node) return false;
    if (auto sem = dynamic_cast<const RESemantic*>(node))
        return grammar->getSemanticName(sem->id()) == "@";
    if (auto term = dynamic_cast<const RETerminal*>(node))
        return term->getID() == 0;
    return false;
}

static std::unique_ptr<RETree> makeEpsilon(Grammar* grammar) {
    return std::make_unique<RETerminal>(grammar, 0);
}

// nil | X = X,  X | nil = X
static std::unique_ptr<RETree> createOr(std::unique_ptr<RETree> a, std::unique_ptr<RETree> b) {
    if (!a) return b;
    if (!b) return a;
    return REOr::make(std::move(a), std::move(b));
}

// nil , X = nil,  X , nil = nil,  ε , X = X,  X , ε = X
static std::unique_ptr<RETree> createAnd(std::unique_ptr<RETree> a,
                                         std::unique_ptr<RETree> b,
                                         Grammar* grammar = nullptr) {
    if (!a || !b) return nullptr;
    if (grammar) {
        if (isEpsilonNode(a.get(), grammar)) return b;
        if (isEpsilonNode(b.get(), grammar)) return a;
    }
    return REAnd::make(std::move(a), std::move(b));
}

// ε | tree
static std::unique_ptr<RETree> createOrEmpty(std::unique_ptr<RETree> tree, Grammar* grammar) {
    return REOr::make(makeEpsilon(grammar), std::move(tree));
}

// ε * tree  (tree* / Kleene star)
static std::unique_ptr<RETree> createUnaryIteration(std::unique_ptr<RETree> tree, Grammar* grammar) {
    if (!tree) return makeEpsilon(grammar);
    return REIteration::make(makeEpsilon(grammar), std::move(tree));
}

// left # right  (general iteration: left·(right·left)*)
// If right is null → returns left (no iteration)
// If left is null  → returns null
static std::unique_ptr<RETree> createIteration(std::unique_ptr<RETree> left,
                                                std::unique_ptr<RETree> right) {
    if (!left)  return nullptr;
    if (!right) return left;
    return REIteration::make(std::move(left), std::move(right));
}

// ---------------------------------------------------------------------------
// computeLeftEl — recursive leftEl decomposition
// Decomposes T into: T = A·R1 | R2
// (analog of TRE_Tree.leftEl virtual method from Pascal)
// ---------------------------------------------------------------------------

static LeftTransformation computeLeftEl(const RETree* node,
                                        const NTListItem* nt,
                                        Grammar* grammar);

static LeftTransformation computeLeftEl(const RETree* node,
                                        const NTListItem* nt,
                                        Grammar* grammar) {
    if (!node) return {};

    const int ntId = grammar->findNonTerminal(nt->name());

    // --- RENonTerminal ---
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->getID() == ntId) {
            // This IS A: T = A·ε | ∅
            return { makeEpsilon(grammar), nullptr, false };
        }
        return { nullptr, node->copy(), false };
    }

    // --- RETerminal ---
    if (auto termNode = dynamic_cast<const RETerminal*>(node)) {
        if (termNode->getID() == 0)
            return { nullptr, nullptr, true };
        return { nullptr, node->copy(), false };
    }

    // --- RESemantic ---
    if (auto semNode = dynamic_cast<const RESemantic*>(node)) {
        if (grammar->getSemanticName(semNode->id()) == "@")
            return { nullptr, nullptr, true };
        return { nullptr, node->copy(), false };
    }

    // --- REOr(L, R) ---
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        auto LTr = computeLeftEl(orNode->left(),  nt, grammar);
        auto RTr = computeLeftEl(orNode->right(), nt, grammar);
        return {
            createOr(std::move(LTr.R1), std::move(RTr.R1)),
            createOr(std::move(LTr.R2), std::move(RTr.R2)),
            LTr.E || RTr.E
        };
    }

    // --- REAnd(L, R) ---
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        const RETree* L = andNode->left();
        const RETree* R = andNode->right();
        if (!L || !R) return { nullptr, node->copy(), false };

        auto LTr = computeLeftEl(L, nt, grammar);

        if (!LTr.E) {
            // L cannot produce epsilon: left recursion only through L's left edge
            return {
                createAnd(std::move(LTr.R1), R->copy(), grammar),
                createAnd(std::move(LTr.R2), R->copy(), grammar),
                false
            };
        } else {
            // L can produce epsilon: also check right operand
            auto RTr = computeLeftEl(R, nt, grammar);
            return {
                createOr(createAnd(std::move(LTr.R1), R->copy(), grammar), std::move(RTr.R1)),
                createOr(createAnd(std::move(LTr.R2), R->copy(), grammar), std::move(RTr.R2)),
                RTr.E
            };
        }
    }

    // --- REIteration(L, R) ---
    if (auto iterNode = dynamic_cast<const REIteration*>(node)) {
        const RETree* L = iterNode->left();
        const RETree* R = iterNode->right();
        if (!L || !R) return { nullptr, node->copy(), false };

        if (isEpsilonNode(L, grammar)) {
            // Unary star: ε*R = R*
            auto RTr = computeLeftEl(R, nt, grammar);
            return {
                createAnd(std::move(RTr.R1), node->copy(), grammar),
                createAnd(std::move(RTr.R2), node->copy(), grammar),
                true
            };
        } else {
            // General L*R → expand to L·(R·L)* then recurse
            auto innerAnd = REAnd::make(R->copy(), L->copy());
            auto iterEps  = createUnaryIteration(std::move(innerAnd), grammar);
            auto temp     = REAnd::make(L->copy(), std::move(iterEps));
            return computeLeftEl(temp.get(), nt, grammar);
        }
    }

    // Unknown type: treat as non-recursive leaf
    return { nullptr, node->copy(), false };
}

// ---------------------------------------------------------------------------
// computeRightEl — recursive rightEl decomposition
// Decomposes T into: T = RA·A | RB
// (analog of TRE_Tree.rightEl virtual method from Pascal)
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
        if (ntNode->getID() == ntId)
            return { makeEpsilon(grammar), nullptr, false };
        return { nullptr, node->copy(), false };
    }

    // --- RETerminal ---
    if (auto termNode = dynamic_cast<const RETerminal*>(node)) {
        if (termNode->getID() == 0)
            return { nullptr, nullptr, true };
        return { nullptr, node->copy(), false };
    }

    // --- RESemantic ---
    if (auto semNode = dynamic_cast<const RESemantic*>(node)) {
        if (grammar->getSemanticName(semNode->id()) == "@")
            return { nullptr, nullptr, true };
        return { nullptr, node->copy(), false };
    }

    // --- REOr(L, R) ---
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        auto LTr = computeRightEl(orNode->left(),  nt, grammar);
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
            return {
                createAnd(L->copy(), std::move(RTr.RA), grammar),
                createAnd(L->copy(), std::move(RTr.RB), grammar),
                false
            };
        } else {
            auto LTr = computeRightEl(L, nt, grammar);
            return {
                createOr(createAnd(L->copy(), std::move(RTr.RA), grammar), std::move(LTr.RA)),
                createOr(createAnd(L->copy(), std::move(RTr.RB), grammar), std::move(LTr.RB)),
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
            auto RTr = computeRightEl(R, nt, grammar);
            return {
                createAnd(node->copy(), std::move(RTr.RA), grammar),
                createAnd(node->copy(), std::move(RTr.RB), grammar),
                true
            };
        } else {
            auto innerAnd = REAnd::make(R->copy(), L->copy());
            auto iterEps  = createUnaryIteration(std::move(innerAnd), grammar);
            auto temp     = REAnd::make(L->copy(), std::move(iterEps));
            return computeRightEl(temp.get(), nt, grammar);
        }
    }

    return { nullptr, node->copy(), false };
}

// ---------------------------------------------------------------------------
// Helper for null-safe rightEl call (used in regularize)
// ---------------------------------------------------------------------------

static RightTransformation safeRightEl(const RETree* tree,
                                       const NTListItem* nt,
                                       Grammar* grammar) {
    if (!tree) return {};
    return computeRightEl(tree, nt, grammar);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Regularize::regularize(Grammar* grammar) {
    if (!grammar) return;

    const int count = static_cast<int>(grammar->getNonTerminals().size());

    // Process in reverse order (as in original Pascal: downto 0)
    for (int i = count - 1; i >= 0; --i) {
        NTListItem* nt = grammar->getNTItemByIndex(i);
        if (!nt || !nt->root()) continue;

        // Step 1: leftEl decomposition → T = A·R1 | R2
        auto tr = computeLeftEl(nt->root(), nt, grammar);

        // Step 2: rightEl on R1 and R2 separately
        // R1 = RA1·A | RB1
        // R2 = RA2·A | RB2
        auto RightT_R1 = safeRightEl(tr.R1.get(), nt, grammar);
        auto RightT_R2 = safeRightEl(tr.R2.get(), nt, grammar);

        // Step 3: assemble the final expression
        //
        // From the Pascal regularize() formula:
        //   core = (RA2)* · RB2 · (RB1)*
        //   if E: core = ε | core
        //   N    = core # RA1  (i.e. core·(RA1·core)*)
        //
        auto core = createAnd(
            createAnd(
                createUnaryIteration(std::move(RightT_R2.RA), grammar),  // (RA2)*
                std::move(RightT_R2.RB),                                  // · RB2
                grammar
            ),
            createUnaryIteration(std::move(RightT_R1.RB), grammar),       // · (RB1)*
            grammar
        );

        if (tr.E && core)
            core = createOrEmpty(std::move(core), grammar);

        auto newRoot = createIteration(std::move(core), std::move(RightT_R1.RA));

        if (newRoot)
            nt->setRoot(std::move(newRoot));
    }
}

} // namespace syngt
