#include <syngt/transform/LeftElimination.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <memory>

namespace syngt {

// ---------------------------------------------------------------------------
// Internal data structure (Pascal TTransformation)
// ---------------------------------------------------------------------------

struct LeftTransformation {
    std::unique_ptr<RETree> R1;  // часть после рекурсивного вхождения A
    std::unique_ptr<RETree> R2;  // нерекурсивная часть
    bool E = false;              // может ли выражение порождать ε
};

// ---------------------------------------------------------------------------
// Helpers (Pascal TransCreator)
// ---------------------------------------------------------------------------

// Является ли узел epsilon (пустым терминалом id=0 или семантикой "@")
static bool isEpsilonNode(const RETree* node, Grammar* grammar) {
    if (!node) return false;
    if (auto term = dynamic_cast<const RETerminal*>(node)) {
        return term->getID() == 0;
    }
    if (auto sem = dynamic_cast<const RESemantic*>(node)) {
        return grammar->getSemanticName(sem->id()) == "@";
    }
    return false;
}

// createEmptyTerminal: RETerminal с id=0 (как в Pascal)
static std::unique_ptr<RETree> makeEpsilon(Grammar* grammar) {
    return std::make_unique<RETerminal>(grammar, 0);
}

// createOr: nil|X=X, X|nil=X, иначе REOr
static std::unique_ptr<RETree> createOr(std::unique_ptr<RETree> a,
                                        std::unique_ptr<RETree> b) {
    if (!a) return b;
    if (!b) return a;
    return REOr::make(std::move(a), std::move(b));
}

// createAnd: nil или nil=nil, ε,X=X, X,ε=X, иначе REAnd
static std::unique_ptr<RETree> createAnd(std::unique_ptr<RETree> a,
                                         std::unique_ptr<RETree> b,
                                         Grammar* grammar) {
    if (!a || !b) return nullptr;
    if (isEpsilonNode(a.get(), grammar)) return b;
    if (isEpsilonNode(b.get(), grammar)) return a;
    return REAnd::make(std::move(a), std::move(b));
}

// createAndAlt: nil,nil=nil, nil,X=X, X,nil=X, ε,X=X, X,ε=X, иначе REAnd
static std::unique_ptr<RETree> createAndAlt(std::unique_ptr<RETree> a,
                                            std::unique_ptr<RETree> b,
                                            Grammar* grammar) {
    if (!a && !b) return nullptr;
    if (!a) return b;
    if (!b) return a;
    if (isEpsilonNode(a.get(), grammar)) return b;
    if (isEpsilonNode(b.get(), grammar)) return a;
    return REAnd::make(std::move(a), std::move(b));
}

// createOrEmpty: nil→ε, isEpsilon→tree, иначе ε|tree
static std::unique_ptr<RETree> createOrEmpty(std::unique_ptr<RETree> tree,
                                              Grammar* grammar) {
    if (!tree) return makeEpsilon(grammar);
    if (isEpsilonNode(tree.get(), grammar)) return tree;
    return REOr::make(makeEpsilon(grammar), std::move(tree));
}

// createUnaryIteration = ε * tree (если tree=nil → ε)
static std::unique_ptr<RETree> createUnaryIteration(std::unique_ptr<RETree> tree,
                                                    Grammar* grammar) {
    if (!tree || isEpsilonNode(tree.get(), grammar)) return makeEpsilon(grammar);
    return REIteration::make(makeEpsilon(grammar), std::move(tree));
}

// ---------------------------------------------------------------------------
// Core recursive decomposition (Pascal TRE_*.leftEl)
// ---------------------------------------------------------------------------

static LeftTransformation computeLeftEl(const RETree* node, int ntId, Grammar* grammar);

static LeftTransformation computeLeftEl(const RETree* node, int ntId, Grammar* grammar) {
    if (!node) return {};

    // --- RETerminal ---
    // isEmpty() = (id == 0): E=true, R1=nil, R2=nil
    // иначе: E=false, R1=nil, R2=self
    if (auto term = dynamic_cast<const RETerminal*>(node)) {
        if (term->getID() == 0) {
            return { nullptr, nullptr, true };
        }
        return { nullptr, node->copy(), false };
    }

    // --- RESemantic ---
    // В C++ системе "@" используется как epsilon; обычная семантика не пустая
    if (auto sem = dynamic_cast<const RESemantic*>(node)) {
        if (grammar->getSemanticName(sem->id()) == "@") {
            return { nullptr, nullptr, true };
        }
        return { nullptr, node->copy(), false };
    }

    // --- RENonTerminal ---
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->getID() == ntId) {
            // Это и есть A: leftEl(A) = { ε, nil, false }
            return { makeEpsilon(grammar), nullptr, false };
        }
        // Другой нетерминал: нет рекурсии
        return { nullptr, node->copy(), false };
    }

    // --- REOr(L, R) ---
    // R1 = Or(L.R1, R.R1), R2 = Or(L.R2, R.R2), E = L.E || R.E
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        auto LTr = computeLeftEl(orNode->left(),  ntId, grammar);
        auto RTr = computeLeftEl(orNode->right(), ntId, grammar);
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

        auto LTr = computeLeftEl(L, ntId, grammar);

        if (!LTr.E) {
            // L не порождает ε
            // R1 = And(L.R1, R), R2 = And(L.R2, R)
            return {
                createAnd(std::move(LTr.R1), R->copy(), grammar),
                createAnd(std::move(LTr.R2), R->copy(), grammar),
                false
            };
        } else {
            // L может порождать ε → учитываем и правую часть
            // R1 = Or(And(L.R1, R), R.R1), R2 = Or(And(L.R2, R), R.R2), E = R.E
            auto RTr = computeLeftEl(R, ntId, grammar);
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
            // Унарная итерация ε*R: E=true
            // R1 = And(R.R1, self), R2 = And(R.R2, self)
            auto RTr = computeLeftEl(R, ntId, grammar);
            return {
                createAnd(std::move(RTr.R1), node->copy(), grammar),
                createAnd(std::move(RTr.R2), node->copy(), grammar),
                true
            };
        } else {
            // Общая итерация L*R → And(L, UnaryIter(And(R,L)))
            auto innerAnd  = REAnd::make(R->copy(), L->copy());
            auto iterEps   = createUnaryIteration(std::move(innerAnd), grammar);
            auto temp      = REAnd::make(L->copy(), std::move(iterEps));
            return computeLeftEl(temp.get(), ntId, grammar);
        }
    }

    // Неизвестный тип → нерекурсивный лист
    return { nullptr, node->copy(), false };
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool LeftElimination::isLeftRecursive(const RETree* node, const NTListItem* nt) {
    if (!node || !nt) return false;

    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->grammar()) {
            auto nts = ntNode->grammar()->getNonTerminals();
            int id = ntNode->getID();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                return nts[id] == nt->name();
            }
        }
        return false;
    }
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        return isLeftRecursive(andNode->left(), nt);
    }
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        return isLeftRecursive(orNode->left(), nt) ||
               isLeftRecursive(orNode->right(), nt);
    }
    return false;
}

bool LeftElimination::hasDirectLeftRecursion(NTListItem* nt) {
    if (!nt) return false;
    RETree* root = nt->root();
    if (!root) return false;
    return isLeftRecursive(root, nt);
}

void LeftElimination::eliminateForNonTerminal(NTListItem* nt, Grammar* grammar) {
    if (!nt || !grammar) return;

    RETree* root = nt->root();
    if (!root) return;

    int ntId = grammar->findNonTerminal(nt->name());
    if (ntId < 0) return;

    // Вычислить трансформацию: TTransformation{R1, R2, E}
    auto tr = computeLeftEl(root, ntId, grammar);

    bool r2WasNonNull = (tr.R2 != nullptr);

    // newRoot = createAndAlt(R2, R1*)
    auto raIter  = createUnaryIteration(std::move(tr.R1), grammar);
    auto newRoot = createAndAlt(std::move(tr.R2), std::move(raIter), grammar);

    // Если E=true и R2 != nil → добавляем epsilon-альтернативу
    if (tr.E && r2WasNonNull) {
        newRoot = createOrEmpty(std::move(newRoot), grammar);
    }

    if (newRoot) {
        nt->setRoot(std::move(newRoot));
    }
}

void LeftElimination::eliminate(Grammar* grammar) {
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
