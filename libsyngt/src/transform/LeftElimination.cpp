#include <syngt/transform/LeftElimination.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/RESemantic.h>

namespace syngt {

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

bool LeftElimination::isLeftRecursive(const RETree* node, const NTListItem* nt) {
    if (!node || !nt) return false;
    
    // Если это нетерминал - проверяем совпадение ID
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
    
    // Если это последовательность (And)
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        return isLeftRecursive(andNode->left(), nt);
    }
    
    // Если это альтернатива (Or)
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        return isLeftRecursive(orNode->left(), nt);
    }
    
    return false;
}

bool LeftElimination::hasDirectLeftRecursion(NTListItem* nt) {
    if (!nt) return false;
    
    RETree* root = nt->root();
    if (!root) return false;
    
    return isLeftRecursive(root, nt);
}

static void collectAlternativesFlat(const RETree* root, std::vector<const RETree*>& alternatives) {
    if (!root) return;
    
    if (auto orNode = dynamic_cast<const REOr*>(root)) {
        collectAlternativesFlat(orNode->left(), alternatives);
        collectAlternativesFlat(orNode->right(), alternatives);
    } else {
        alternatives.push_back(root);
    }
}

void LeftElimination::collectAlphaAndBeta(
    const RETree* root,
    const NTListItem* nt,
    std::vector<std::unique_ptr<RETree>>& alphaList,
    std::vector<std::unique_ptr<RETree>>& betaList
) {
    if (!root) return;
    
    // Сначала собираем ВСЕ альтернативы в плоский список
    std::vector<const RETree*> alternatives;
    collectAlternativesFlat(root, alternatives);
    
    // Теперь для каждой альтернативы проверяем леворекурсивность
    for (const RETree* alt : alternatives) {
        if (isLeftRecursive(alt, nt)) {
            auto alpha = extractAlpha(alt, nt);
            alphaList.push_back(std::move(alpha));
        } else {
            betaList.push_back(alt->copy());
        }
    }
}

std::unique_ptr<RETree> LeftElimination::extractAlpha(const RETree* node, const NTListItem* nt) {
    if (!node || !nt) return nullptr;
    
    // Базовый случай: это сам нетерминал A (без α)
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->grammar()) {
            auto nts = ntNode->grammar()->getNonTerminals();
            int id = ntNode->getID();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                if (nts[id] == nt->name()) {
                    return nullptr;  // α = ε
                }
            }
        }
    }
    
    // Если это последовательность And
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        // Проверяем, рекурсивна ли левая часть
        if (isLeftRecursive(andNode->left(), nt)) {
            // Рекурсивно извлекаем α из левой части
            auto leftAlpha = extractAlpha(andNode->left(), nt);
            
            // Правая часть - всегда часть α
            auto rightCopy = andNode->right()->copy();
            
            if (leftAlpha) {
                // Комбинируем: leftAlpha , right
                return REAnd::make(std::move(leftAlpha), std::move(rightCopy));
            } else {
                // Левая часть = просто A, α начинается с right
                return rightCopy;
            }
        }
    }
    
    // Не рекурсивный случай
    return nullptr;
}

std::unique_ptr<RETree> LeftElimination::buildBetaWithRecursion(
    std::vector<std::unique_ptr<RETree>>& betaList,
    const std::string& recursiveName,
    Grammar* grammar
) {
    if (betaList.empty()) return nullptr;
    
    int recId = grammar->findNonTerminal(recursiveName);
    if (recId < 0) return nullptr;
    
    std::unique_ptr<RETree> result;
    
    for (auto& beta : betaList) {
        // β , A'
        auto recNT = std::make_unique<RENonTerminal>(grammar, recId, false);
        auto betaWithRec = REAnd::make(std::move(beta), std::move(recNT));
        
        // Объединяем через Or
        if (!result) {
            result = std::move(betaWithRec);
        } else {
            result = REOr::make(std::move(result), std::move(betaWithRec));
        }
    }
    
    return result;
}

std::unique_ptr<RETree> LeftElimination::buildAlphaWithRecursion(
    std::vector<std::unique_ptr<RETree>>& alphaList,
    const std::string& recursiveName,
    Grammar* grammar
) {
    int recId = grammar->findNonTerminal(recursiveName);
    if (recId < 0) return nullptr;

// В начале buildAlphaWithRecursion после проверки recId
bool allEmpty = true;
for (const auto& alpha : alphaList) {
    if (alpha) {
        allEmpty = false;
        break;
    }
}

// Если все α пустые, A' просто не нужен - возвращаем только epsilon
if (allEmpty) {
    int epsilonId = grammar->addSemantic("@");
    return std::make_unique<RESemantic>(grammar, epsilonId);
}
    
    std::unique_ptr<RETree> result;
    
    // Строим α A' для каждой α
    for (auto& alpha : alphaList) {
        auto recNT = std::make_unique<RENonTerminal>(grammar, recId, false);
        
        std::unique_ptr<RETree> alphaWithRec;
        if (alpha) {
            // α , A'
            alphaWithRec = REAnd::make(std::move(alpha), std::move(recNT));
        } else {
            // Просто A' (если α была пустой)
            alphaWithRec = std::move(recNT);
        }
        
        // Объединяем через Or
        if (!result) {
            result = std::move(alphaWithRec);
        } else {
            result = REOr::make(std::move(result), std::move(alphaWithRec));
        }
    }
    
    int epsilonId = grammar->addSemantic("@");
    auto epsilon = std::make_unique<RESemantic>(grammar, epsilonId);
    
    if (result) {
        result = REOr::make(std::move(result), std::move(epsilon));
    } else {
        result = std::move(epsilon);
    }
    
    return result;
}

}

#include <iostream> // ДОБАВИТЬ В НАЧАЛО ФАЙЛА!

void syngt::LeftElimination::eliminateForNonTerminal(NTListItem* nt, Grammar* grammar) {
    if (!nt || !grammar) return;
    if (!hasDirectLeftRecursion(nt)) return;
    
    RETree* root = nt->root();
    if (!root) return;
    
    // Списки для α (рекурсивные) и β (нерекурсивные) части
    std::vector<std::unique_ptr<RETree>> alphaList;
    std::vector<std::unique_ptr<RETree>> betaList;
    
    // Разделяем правило
    collectAlphaAndBeta(root, nt, alphaList, betaList);
    
    // Если нет нерекурсивных альтернатив - невозможно устранить
    if (betaList.empty()) {
        return;
    }
    
    // Создаем новый нетерминал A'
    std::string newName = nt->name() + "_rec";
    grammar->addNonTerminal(newName);
    
    // Строим A → β A'
    auto newRoot = buildBetaWithRecursion(betaList, newName, grammar);
    if (newRoot) {
        nt->setRoot(std::move(newRoot));
    }
    
    NTListItem* newNT = grammar->getNTItem(newName);
    if (newNT) {
        auto newNTRoot = buildAlphaWithRecursion(alphaList, newName, grammar);
        if (newNTRoot) {
            newNT->setRoot(std::move(newNTRoot));
        }
    }
}