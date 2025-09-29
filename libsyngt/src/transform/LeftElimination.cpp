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
    
    // Если нет нерекурсивных альтернатив - ошибка в грамматике
    if (betaList.empty()) {
        return;  // Невозможно устранить
    }
    
    // Создаем новый нетерминал A'
    std::string newName = nt->name() + "_rec";
    grammar->addNonTerminal(newName);
    
    // Строим A → β A'
    auto newRoot = buildBetaWithRecursion(betaList, newName, grammar);
    nt->setRoot(std::move(newRoot));
    
    // Строим A' → α A' | ε
    NTListItem* newNT = grammar->getNTItem(newName);
    if (newNT) {
        auto newNTRoot = buildAlphaWithRecursion(alphaList, newName, grammar);
        newNT->setRoot(std::move(newNTRoot));
    }
}

bool LeftElimination::hasDirectLeftRecursion(NTListItem* nt) {
    if (!nt) return false;
    
    RETree* root = nt->root();
    if (!root) return false;
    
    // Проверяем прямую левую рекурсию: A → A α
    return isLeftRecursive(root, nt);
}

bool LeftElimination::isLeftRecursive(const RETree* node, const NTListItem* nt) {
    if (!node || !nt) return false;
    
    // Если это нетерминал
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        // Сравниваем имена нетерминалов
        return ntNode->nameID() == nt->name();
    }
    
    // Если это последовательность (And)
    if (auto andNode = dynamic_cast<const REAnd*>(node)) {
        // Проверяем первый операнд
        return isLeftRecursive(andNode->left(), nt);
    }
    
    // Если это альтернатива (Or)
    if (auto orNode = dynamic_cast<const REOr*>(node)) {
        // Проверяем хотя бы одну альтернативу
        return isLeftRecursive(orNode->left(), nt) || 
               isLeftRecursive(orNode->right(), nt);
    }
    
    return false;
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
            // Это рекурсивная альтернатива → добавляем в alpha
            auto alpha = extractAlpha(orNode->left(), nt);
            if (alpha) {
                alphaList.push_back(std::move(alpha));
            }
        } else {
            // Это нерекурсивная альтернатива → добавляем в beta
            betaList.push_back(orNode->left()->copy());
        }
        
        // Рекурсивно обрабатываем правую часть
        collectAlphaAndBeta(orNode->right(), nt, alphaList, betaList);
    }
    else {
        // Одиночное правило
        if (isLeftRecursive(root, nt)) {
            auto alpha = extractAlpha(root, nt);
            if (alpha) {
                alphaList.push_back(std::move(alpha));
            }
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
    
    // Если это просто A (без α)
    if (auto ntNode = dynamic_cast<const RENonTerminal*>(node)) {
        if (ntNode->nameID() == nt->name()) {
            return nullptr;  // Нет α части (просто epsilon)
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
    
    // Создаем нетерминал A'
    int recId = grammar->findNonTerminal(recursiveName);
    auto recNT = std::make_unique<RENonTerminal>(grammar, recId, false);
    
    // Строим β A' для каждой β
    std::unique_ptr<RETree> result;
    
    for (auto& beta : betaList) {
        // β , A'
        auto recCopy = std::make_unique<RENonTerminal>(grammar, recId, false);
        auto betaWithRec = REAnd::make(std::move(beta), std::move(recCopy));
        
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
    if (alphaList.empty()) {
        // A' → ε (пустое правило)
        int epsilonId = grammar->addSemantic("@");
        return std::make_unique<RESemantic>(grammar, epsilonId);
    }
    
    int recId = grammar->findNonTerminal(recursiveName);
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
    result = REOr::make(std::move(result), std::move(epsilon));
    
    return result;
}

}