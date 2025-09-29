#include <syngt/transform/RemoveUseless.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <queue>
#include <set>
#include <iostream>

namespace syngt {

static bool checkTreeProductive(const RETree* tree, const std::set<int>& productive) {
    if (!tree) return true;
    
    // Терминал всегда продуктивен
    if (dynamic_cast<const RETerminal*>(tree)) {
        return true;
    }
    
    // Семантика всегда продуктивна
    if (dynamic_cast<const RESemantic*>(tree)) {
        return true;
    }
    
    // Нетерминал продуктивен если в списке продуктивных
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        return productive.count(nt->getID()) > 0;
    }
    
    // Or продуктивен если хотя бы одна ветка продуктивна
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        return checkTreeProductive(orNode->left(), productive) ||
               checkTreeProductive(orNode->right(), productive);
    }
    
    // And продуктивен если обе ветки продуктивны
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        return checkTreeProductive(andNode->left(), productive) &&
               checkTreeProductive(andNode->right(), productive);
    }
    
    // Iteration всегда продуктивен (может быть 0 повторений)
    if (dynamic_cast<const REIteration*>(tree)) {
        return true;
    }
    
    return true;
}

static void collectUsedIndices(const RETree* tree, std::set<int>& used) {
    if (!tree) return;
    
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        used.insert(nt->getID());
        return;
    }
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        collectUsedIndices(orNode->left(), used);
        collectUsedIndices(orNode->right(), used);
    }
    
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        collectUsedIndices(andNode->left(), used);
        collectUsedIndices(andNode->right(), used);
    }
    
    if (auto iterNode = dynamic_cast<const REIteration*>(tree)) {
        collectUsedIndices(iterNode->left(), used);
        collectUsedIndices(iterNode->right(), used);
    }
}

void RemoveUseless::remove(Grammar* grammar) {
    if (!grammar) return;
    
    auto allNTs = grammar->getNonTerminals();
    int ntCount = static_cast<int>(allNTs.size());
    
    // ШАГ 1: Найти продуктивные символы
    std::set<int> productive;
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (int i = 0; i < ntCount; ++i) {
            if (productive.count(i) > 0) continue;
            
            NTListItem* nt = grammar->getNTItemByIndex(i);
            if (!nt || !nt->hasRoot()) continue;
            
            if (checkTreeProductive(nt->root(), productive)) {
                productive.insert(i);
                changed = true;
            }
        }
    }
    
    // ШАГ 2: Удалить непродуктивные
    for (int i = 0; i < ntCount; ++i) {
        if (productive.count(i) == 0) {
            NTListItem* nt = grammar->getNTItemByIndex(i);
            if (nt && nt->hasRoot()) {
                nt->setRoot(nullptr);
            }
        }
    }
    
    // ШАГ 3: Найти достижимые символы (среди оставшихся продуктивных)
    std::set<int> reachable;
    std::queue<int> toVisit;
    
    // Найти первый нетерминал с правилом (стартовый)
    int startIdx = -1;
    for (int i = 0; i < ntCount; ++i) {
        NTListItem* nt = grammar->getNTItemByIndex(i);
        if (nt && nt->hasRoot()) {
            startIdx = i;
            break;
        }
    }
    
    if (startIdx < 0) {
        return;  // Нет стартового символа
    }
    
    reachable.insert(startIdx);
    toVisit.push(startIdx);
    
    // BFS
    while (!toVisit.empty()) {
        int current = toVisit.front();
        toVisit.pop();
        
        NTListItem* nt = grammar->getNTItemByIndex(current);
        if (!nt || !nt->hasRoot()) continue;
        
        std::set<int> used;
        collectUsedIndices(nt->root(), used);
        
        for (int idx : used) {
            // Добавляем только если продуктивен и еще не достигнут
            if (productive.count(idx) > 0 && reachable.count(idx) == 0) {
                reachable.insert(idx);
                toVisit.push(idx);
            }
        }
    }
    
    // ШАГ 4: Удалить недостижимые
    for (int i = 0; i < ntCount; ++i) {
        if (reachable.count(i) == 0) {
            NTListItem* nt = grammar->getNTItemByIndex(i);
            if (nt && nt->hasRoot()) {
                nt->setRoot(nullptr);
            }
        }
    }
}

}