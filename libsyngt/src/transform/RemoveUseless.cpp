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
    
    if (dynamic_cast<const RETerminal*>(tree)) {
        return true;
    }
    
    if (dynamic_cast<const RESemantic*>(tree)) {
        return true;
    }
    
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        return productive.count(nt->getID()) > 0;
    }
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        return checkTreeProductive(orNode->left(), productive) ||
               checkTreeProductive(orNode->right(), productive);
    }
    
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        return checkTreeProductive(andNode->left(), productive) &&
               checkTreeProductive(andNode->right(), productive);
    }
    
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
    
    for (int i = 0; i < ntCount; ++i) {
        if (productive.count(i) == 0) {
            NTListItem* nt = grammar->getNTItemByIndex(i);
            if (nt && nt->hasRoot()) {
                nt->setRoot(nullptr);
            }
        }
    }
    
    std::set<int> reachable;
    std::queue<int> toVisit;
    
    int startIdx = -1;
    for (int i = 0; i < ntCount; ++i) {
        NTListItem* nt = grammar->getNTItemByIndex(i);
        if (nt && nt->hasRoot()) {
            startIdx = i;
            break;
        }
    }
    
    if (startIdx < 0) {
        return;
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
            if (productive.count(idx) > 0 && reachable.count(idx) == 0) {
                reachable.insert(idx);
                toVisit.push(idx);
            }
        }
    }
    
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