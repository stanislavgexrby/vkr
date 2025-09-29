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

void LeftElimination::collectAlphaAndBeta(
    const RETree* root,
    const NTListItem* nt,
    std::vector<std::unique_ptr<RETree>>& alphaList,
    std::vector<std::unique_ptr<RETree>>& betaList
) {
    if (!root) return;
    
    // Если это альтернатива (Or)
    if (auto orNode = dynamic_cast<const REOr*>(root)) {
        // Обрабатываем левую часть
        if (isLeftRecursive(orNode->left(), nt)) {
            // Рекурсивная альтернатива → добавляем в alpha
            auto alpha = extractAlpha(orNode->left(), nt);
            alphaList.push_back(std::move(alpha));
        } else {
            // Нерекурсивная альтернатива → добавляем в beta
            betaList.push_back(orNode->left()->copy());
        }
        
        // Рекурсивно обрабатываем правую часть
        collectAlphaAndBeta(orNode->right(), nt, alphaList, betaList);
    }
    else {
        // Одиночное правило
        if (isLeftRecursive(root, nt)) {
            auto alpha = extractAlpha(root, nt);
            alphaList.push_back(std::move(alpha));
        } else {
            betaList.push_back(root->copy());
        }
    }
}

std::unique_ptr<RETree> LeftElimination::extractAlpha(const RETree* node, const NTListItem* nt) {
    if (!node || !nt) return nullptr;
    
    // Если это последовательность: A, α
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        if (isLeftRecursive(andNode->left(), nt)) {
            // Возвращаем α (правую часть)
            return andNode->right()->copy();
        }
    }
    
    // Если это просто A (без α) - возвращаем nullptr (будет epsilon)
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->grammar()) {
            auto nts = ntNode->grammar()->getNonTerminals();
            int id = ntNode->getID();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                if (nts[id] == nt->name()) {
                    return nullptr;  // Просто A, α пустая
                }
            }
        }
    }
    
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

void LeftElimination::eliminateForNonTerminal(NTListItem* nt, Grammar* grammar) {
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

}