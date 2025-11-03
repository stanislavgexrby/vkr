#include <syngt/transform/FirstFollow.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <iostream>
#include <functional>

namespace syngt {

struct NullableInfo {
    std::map<std::string, bool> nullable;
    
    bool isNullable(const std::string& ntName) const {
        auto it = nullable.find(ntName);
        return it != nullable.end() && it->second;
    }
};

static NullableInfo computeNullable(Grammar* grammar) {
    NullableInfo info;
    auto nts = grammar->getNonTerminals();
    
    for (const auto& nt : nts) {
        info.nullable[nt] = false;
    }
    
    bool changed = true;
    int iterations = 0;
    
    while (changed && iterations < 100) {
        changed = false;
        iterations++;
        
        for (const auto& ntName : nts) {
            if (info.nullable[ntName]) continue;
            
            NTListItem* nt = grammar->getNTItem(ntName);
            if (!nt || !nt->hasRoot()) continue;
            
            std::function<bool(const RETree*)> checkNullable = [&](const RETree* tree) -> bool {
                if (!tree) return true;
                
                // @ - epsilon
                if (dynamic_cast<const RESemantic*>(tree)) return true;
                
                if (dynamic_cast<const RETerminal*>(tree)) return false;
                
                if (dynamic_cast<const REIteration*>(tree)) return true;
                
                if (auto ntNode = dynamic_cast<const RENonTerminal*>(tree)) {
                    if (ntNode->grammar()) {
                        auto nts2 = ntNode->grammar()->getNonTerminals();
                        int id = ntNode->getID();
                        if (id >= 0 && id < static_cast<int>(nts2.size())) {
                            return info.nullable[nts2[id]];
                        }
                    }
                    return false;
                }
                
                if (auto orNode = dynamic_cast<const REOr*>(tree)) {
                    return checkNullable(orNode->left()) || checkNullable(orNode->right());
                }
                
                if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
                    return checkNullable(andNode->left()) && checkNullable(andNode->right());
                }
                
                return false;
            };
            
            bool isNullableNow = checkNullable(nt->root());
            if (isNullableNow && !info.nullable[ntName]) {
                info.nullable[ntName] = true;
                changed = true;
            }
        }
    }
    
    return info;
}

std::map<std::string, FirstFollow::TerminalSet> FirstFollow::computeFirst(Grammar* grammar) {
    if (!grammar) return {};
    
    std::map<std::string, TerminalSet> firstSets;
    auto nts = grammar->getNonTerminals();
    
    NullableInfo nullableInfo = computeNullable(grammar);
    
    for (const auto& nt : nts) {
        firstSets[nt] = TerminalSet();
    }
    
    bool changed = true;
    int iterations = 0;
    
    while (changed && iterations < 100) {
        changed = false;
        iterations++;
        
        for (const auto& ntName : nts) {
            NTListItem* nt = grammar->getNTItem(ntName);
            if (!nt || !nt->hasRoot()) continue;
            
            size_t oldSize = firstSets[ntName].size();
            
            std::function<TerminalSet(const RETree*)> getFirst = [&](const RETree* tree) -> TerminalSet {
                TerminalSet result;
                if (!tree) return result;
                
                if (auto term = dynamic_cast<const RETerminal*>(tree)) {
                    result.insert(term->getID());
                    return result;
                }
                
                if (dynamic_cast<const RESemantic*>(tree)) {
                    return result;
                }
                
                if (auto ntNode = dynamic_cast<const RENonTerminal*>(tree)) {
                    if (ntNode->grammar()) {
                        auto nts2 = ntNode->grammar()->getNonTerminals();
                        int id = ntNode->getID();
                        if (id >= 0 && id < static_cast<int>(nts2.size())) {
                            std::string name = nts2[id];
                            if (firstSets.count(name) > 0) {
                                result = firstSets[name];
                            }
                        }
                    }
                    return result;
                }
                
                if (auto orNode = dynamic_cast<const REOr*>(tree)) {
                    auto left = getFirst(orNode->left());
                    auto right = getFirst(orNode->right());
                    result.insert(left.begin(), left.end());
                    result.insert(right.begin(), right.end());
                    return result;
                }
                
                if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
                    auto left = getFirst(andNode->left());
                    result.insert(left.begin(), left.end());
                    
                    bool leftNullable = false;
                    if (auto ntNode = dynamic_cast<const RENonTerminal*>(andNode->left())) {
                        if (ntNode->grammar()) {
                            auto nts2 = ntNode->grammar()->getNonTerminals();
                            int id = ntNode->getID();
                            if (id >= 0 && id < static_cast<int>(nts2.size())) {
                                leftNullable = nullableInfo.isNullable(nts2[id]);
                            }
                        }
                    } else if (dynamic_cast<const RESemantic*>(andNode->left()) ||
                               dynamic_cast<const REIteration*>(andNode->left())) {
                        leftNullable = true;
                    }
                    
                    if (leftNullable) {
                        auto right = getFirst(andNode->right());
                        result.insert(right.begin(), right.end());
                    }
                    
                    return result;
                }
                
                if (auto iterNode = dynamic_cast<const REIteration*>(tree)) {
                    return getFirst(iterNode->left());
                }
                
                return result;
            };
            
            auto newFirst = getFirst(nt->root());
            firstSets[ntName].insert(newFirst.begin(), newFirst.end());
            
            if (firstSets[ntName].size() != oldSize) {
                changed = true;
            }
        }
    }
    
    return firstSets;
}

std::map<std::string, FirstFollow::TerminalSet> FirstFollow::computeFollow(
    Grammar* grammar,
    const std::map<std::string, TerminalSet>& firstSets
) {
    if (!grammar) return {};
    
    std::map<std::string, TerminalSet> followSets;
    auto nts = grammar->getNonTerminals();
    
    NullableInfo nullableInfo = computeNullable(grammar);
    
    for (const auto& nt : nts) {
        followSets[nt] = TerminalSet();
    }
    
    if (!nts.empty()) {
        followSets[nts[0]].insert(-1);  // $ = EOF
    }
    
    bool changed = true;
    int iterations = 0;
    
    while (changed && iterations < 100) {
        changed = false;
        iterations++;
        
        for (const auto& ntNameA : nts) {
            NTListItem* ntA = grammar->getNTItem(ntNameA);
            if (!ntA || !ntA->hasRoot()) continue;
            
            std::function<void(const RETree*, bool)> analyzeTree = [&](const RETree* tree, bool afterNullable) {
                if (!tree) return;
                
                if (auto ntB = dynamic_cast<const RENonTerminal*>(tree)) {
                    if (ntB->grammar()) {
                        auto nts2 = ntB->grammar()->getNonTerminals();
                        int id = ntB->getID();
                        if (id >= 0 && id < static_cast<int>(nts2.size())) {
                            std::string nameB = nts2[id];
                            
                            if (afterNullable) {
                                size_t oldSize = followSets[nameB].size();
                                followSets[nameB].insert(followSets[ntNameA].begin(), followSets[ntNameA].end());
                                if (followSets[nameB].size() != oldSize) {
                                    changed = true;
                                }
                            }
                        }
                    }
                    return;
                }
                
                // And: A → ... B β
                if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
                    if (auto ntB = dynamic_cast<const RENonTerminal*>(andNode->left())) {
                        if (ntB->grammar()) {
                            auto nts2 = ntB->grammar()->getNonTerminals();
                            int id = ntB->getID();
                            if (id >= 0 && id < static_cast<int>(nts2.size())) {
                                std::string nameB = nts2[id];
                                
                                // FOLLOW(B) += FIRST(β)
                                std::function<TerminalSet(const RETree*)> getFirst = [&](const RETree* t) -> TerminalSet {
                                    TerminalSet res;
                                    if (!t) return res;
                                    
                                    if (auto term = dynamic_cast<const RETerminal*>(t)) {
                                        res.insert(term->getID());
                                    } else if (auto nt = dynamic_cast<const RENonTerminal*>(t)) {
                                        if (nt->grammar()) {
                                            auto nts3 = nt->grammar()->getNonTerminals();
                                            int id2 = nt->getID();
                                            if (id2 >= 0 && id2 < static_cast<int>(nts3.size())) {
                                                if (firstSets.count(nts3[id2]) > 0) {
                                                    res = firstSets.at(nts3[id2]);
                                                }
                                            }
                                        }
                                    }
                                    return res;
                                };
                                
                                auto firstBeta = getFirst(andNode->right());
                                size_t oldSize = followSets[nameB].size();
                                followSets[nameB].insert(firstBeta.begin(), firstBeta.end());
                                if (followSets[nameB].size() != oldSize) {
                                    changed = true;
                                }
                                
                                // β - nullable, FOLLOW(B) += FOLLOW(A)
                                bool betaNullable = false;
                                if (auto ntBeta = dynamic_cast<const RENonTerminal*>(andNode->right())) {
                                    if (ntBeta->grammar()) {
                                        auto nts3 = ntBeta->grammar()->getNonTerminals();
                                        int id2 = ntBeta->getID();
                                        if (id2 >= 0 && id2 < static_cast<int>(nts3.size())) {
                                            betaNullable = nullableInfo.isNullable(nts3[id2]);
                                        }
                                    }
                                } else if (dynamic_cast<const RESemantic*>(andNode->right()) ||
                                           dynamic_cast<const REIteration*>(andNode->right())) {
                                    betaNullable = true;
                                }
                                
                                if (betaNullable) {
                                    oldSize = followSets[nameB].size();
                                    followSets[nameB].insert(followSets[ntNameA].begin(), followSets[ntNameA].end());
                                    if (followSets[nameB].size() != oldSize) {
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    analyzeTree(andNode->left(), false);
                    
                    bool rightNullable = false;
                    if (auto nt = dynamic_cast<const RENonTerminal*>(andNode->right())) {
                        if (nt->grammar()) {
                            auto nts2 = nt->grammar()->getNonTerminals();
                            int id = nt->getID();
                            if (id >= 0 && id < static_cast<int>(nts2.size())) {
                                rightNullable = nullableInfo.isNullable(nts2[id]);
                            }
                        }
                    } else if (dynamic_cast<const RESemantic*>(andNode->right()) ||
                               dynamic_cast<const REIteration*>(andNode->right())) {
                        rightNullable = true;
                    }
                    
                    analyzeTree(andNode->right(), afterNullable || rightNullable);
                    return;
                }
                
                // Or
                if (auto orNode = dynamic_cast<const REOr*>(tree)) {
                    analyzeTree(orNode->left(), afterNullable);
                    analyzeTree(orNode->right(), afterNullable);
                    return;
                }
                
                // Iteration
                if (auto iterNode = dynamic_cast<const REIteration*>(tree)) {
                    analyzeTree(iterNode->left(), true);
                }
            };
            
            analyzeTree(ntA->root(), true);
        }
    }
    
    return followSets;
}

bool FirstFollow::isLL1(Grammar* grammar) {
    if (!grammar) return false;
    
    auto firstSets = computeFirst(grammar);
    auto followSets = computeFollow(grammar, firstSets);
    
    auto nts = grammar->getNonTerminals();
    for (const auto& ntName : nts) {
        NTListItem* nt = grammar->getNTItem(ntName);
        if (!nt || !nt->hasRoot()) continue;
        
        if (!checkLL1ForNT(nt, firstSets, followSets)) {
            return false;
        }
    }
    
    return true;
}

bool FirstFollow::checkLL1ForNT(
    NTListItem* nt,
    const std::map<std::string, TerminalSet>& firstSets,
    const std::map<std::string, TerminalSet>& followSets
) {
    if (!nt || !nt->hasRoot()) return true;
    
    std::vector<const RETree*> alternatives;
    collectAlternatives(nt->root(), alternatives);
    
    if (alternatives.size() < 2) return true;
    
    NullableInfo nullableInfo;

    for (size_t i = 0; i < alternatives.size(); ++i) {
        bool nullable1 = false;
        auto first1 = computeFirstForTree(alternatives[i], firstSets, nullable1);
        
        for (size_t j = i + 1; j < alternatives.size(); ++j) {
            bool nullable2 = false;
            auto first2 = computeFirstForTree(alternatives[j], firstSets, nullable2);
            
            for (int term : first1) {
                if (first2.count(term) > 0) {
                    return false;
                }
            }
            
            if (nullable1 && followSets.count(nt->name()) > 0) {
                const auto& follow = followSets.at(nt->name());
                for (int term : first2) {
                    if (follow.count(term) > 0) {
                        return false;
                    }
                }
            }
            
            if (nullable2 && followSets.count(nt->name()) > 0) {
                const auto& follow = followSets.at(nt->name());
                for (int term : first1) {
                    if (follow.count(term) > 0) {
                        return false;
                    }
                }
            }
        }
    }
    
    return true;
}

FirstFollow::TerminalSet FirstFollow::computeFirstForTree(
    const RETree* tree,
    const std::map<std::string, TerminalSet>& knownFirst,
    bool& nullable
) {
    TerminalSet result;
    nullable = false;
    
    if (!tree) {
        nullable = true;
        return result;
    }
    
    if (auto term = dynamic_cast<const RETerminal*>(tree)) {
        result.insert(term->getID());
        return result;
    }
    
    if (dynamic_cast<const RESemantic*>(tree)) {
        nullable = true;
        return result;
    }
    
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        if (nt->grammar()) {
            auto nts = nt->grammar()->getNonTerminals();
            int id = nt->getID();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                std::string name = nts[id];
                if (knownFirst.count(name) > 0) {
                    result = knownFirst.at(name);
                }
            }
        }
        return result;
    }
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        bool n1, n2;
        auto f1 = computeFirstForTree(orNode->left(), knownFirst, n1);
        auto f2 = computeFirstForTree(orNode->right(), knownFirst, n2);
        result.insert(f1.begin(), f1.end());
        result.insert(f2.begin(), f2.end());
        nullable = n1 || n2;
        return result;
    }
    
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        bool n1;
        auto f1 = computeFirstForTree(andNode->left(), knownFirst, n1);
        result.insert(f1.begin(), f1.end());
        
        if (n1) {
            bool n2;
            auto f2 = computeFirstForTree(andNode->right(), knownFirst, n2);
            result.insert(f2.begin(), f2.end());
            nullable = n2;
        }
        return result;
    }
    
    if (auto iterNode = dynamic_cast<const REIteration*>(tree)) {
        nullable = true;
        bool dummy;
        return computeFirstForTree(iterNode->left(), knownFirst, dummy);
    }
    
    return result;
}

void FirstFollow::collectAlternatives(
    const RETree* tree,
    std::vector<const RETree*>& alternatives
) {
    if (!tree) return;
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        collectAlternatives(orNode->left(), alternatives);
        collectAlternatives(orNode->right(), alternatives);
    } else {
        alternatives.push_back(tree);
    }
}

void FirstFollow::printSets(
    Grammar* grammar,
    const std::map<std::string, TerminalSet>& firstSets,
    const std::map<std::string, TerminalSet>& followSets
) {
    if (!grammar) return;
    
    std::cout << "\n=== FIRST Sets ===\n";
    for (const auto& [ntName, firstSet] : firstSets) {
        std::cout << ntName << ": { ";
        for (int termId : firstSet) {
            if (termId >= 0 && termId < grammar->terminals()->getCount()) {
                std::cout << grammar->terminals()->getString(termId) << " ";
            }
        }
        std::cout << "}\n";
    }
    
    std::cout << "\n=== FOLLOW Sets ===\n";
    for (const auto& [ntName, followSet] : followSets) {
        std::cout << ntName << ": { ";
        for (int termId : followSet) {
            if (termId == -1) {
                std::cout << "$ ";
            } else if (termId >= 0 && termId < grammar->terminals()->getCount()) {
                std::cout << grammar->terminals()->getString(termId) << " ";
            }
        }
        std::cout << "}\n";
    }
}

bool FirstFollow::isNullable(
    const RETree* tree,
    const std::map<std::string, bool>& knownNullable
) {
    if (!tree) return true;
    if (dynamic_cast<const RESemantic*>(tree)) return true;
    if (dynamic_cast<const RETerminal*>(tree)) return false;
    if (dynamic_cast<const REIteration*>(tree)) return true;
    
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        if (nt->grammar()) {
            auto nts = nt->grammar()->getNonTerminals();
            int id = nt->getID();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                std::string name = nts[id];
                if (knownNullable.count(name) > 0) {
                    return knownNullable.at(name);
                }
            }
        }
        return false;
    }
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        return isNullable(orNode->left(), knownNullable) ||
               isNullable(orNode->right(), knownNullable);
    }
    
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        return isNullable(andNode->left(), knownNullable) &&
               isNullable(andNode->right(), knownNullable);
    }
    
    return false;
}

}