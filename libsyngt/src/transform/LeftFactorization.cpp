#include <syngt/transform/LeftFactorization.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <algorithm>
#include <map>
#include <set>

#include <iostream>

namespace syngt {

void LeftFactorization::factorizeAll(Grammar* grammar) {
    if (!grammar) return;
    
    auto nts = grammar->getNonTerminals();
    for (size_t i = 0; i < nts.size(); ++i) {
        NTListItem* nt = grammar->getNTItemByIndex(static_cast<int>(i));
        if (nt) {
            factorize(nt, grammar);
        }
    }
}

void LeftFactorization::factorize(NTListItem* nt, Grammar* grammar) {
    if (!nt || !grammar) return;
    
    // Повторяем факторизацию пока есть изменения
    const int maxIterations = 10;
    
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        RETree* root = nt->root();
        if (!root) {
            return;
        }
        
        // Собираем все альтернативы
        std::vector<const RETree*> alternatives;
        collectAlternatives(root, alternatives);
        
        if (alternatives.size() < 2) {
            return;
        }
        
        if (!hasCommonPrefixes(alternatives)) {
            return;
        }
        
        // Группировка
        auto groups = groupByPrefix(alternatives);
        
        if (groups.size() == alternatives.size()) {
            return;
        }
        
        auto newRule = buildFactorizedRule(groups, grammar);
        if (!newRule) {
            return;
        }
        
        nt->setRoot(std::move(newRule));
    }
}

void LeftFactorization::collectAlternatives(
    const RETree* root,
    std::vector<const RETree*>& alternatives
) {
    if (!root) return;
    
    if (auto orNode = dynamic_cast<const REOr*>(root)) {
        collectAlternatives(orNode->left(), alternatives);
        collectAlternatives(orNode->right(), alternatives);
    } else {
        alternatives.push_back(root);
    }
}

// Развернуть And-цепочку в плоский список
static void flattenAndChain(const RETree* root, std::vector<const RETree*>& result) {
    if (auto andNode = dynamic_cast<const REAnd*>(root)) {
        flattenAndChain(andNode->left(), result);
        flattenAndChain(andNode->right(), result);
    } else {
        result.push_back(root);
    }
}

// Собрать список обратно в And-цепочку
static std::unique_ptr<RETree> buildAndChain(const std::vector<const RETree*>& list, size_t start, size_t end) {
    if (start >= end) return nullptr;
    if (start + 1 == end) return list[start]->copy();
    
    auto result = list[start]->copy();
    for (size_t i = start + 1; i < end; ++i) {
        result = REAnd::make(std::move(result), list[i]->copy());
    }
    return result;
}

std::unique_ptr<RETree> LeftFactorization::findCommonPrefix(
    const RETree* tree1,
    const RETree* tree2
) {
    if (!tree1 || !tree2) return nullptr;
    
    // Развернуть обе в списки
    std::vector<const RETree*> list1, list2;
    flattenAndChain(tree1, list1);
    flattenAndChain(tree2, list2);
    
    // Найти наибольший общий префикс
    size_t commonLen = 0;
    while (commonLen < list1.size() && commonLen < list2.size() &&
           treesEqual(list1[commonLen], list2[commonLen])) {
        commonLen++;
    }
    
    if (commonLen == 0) return nullptr;
    
    // Собрать обратно
    return buildAndChain(list1, 0, commonLen);
}

bool LeftFactorization::hasCommonPrefixes(
    const std::vector<const RETree*>& alternatives
) {
    for (size_t i = 0; i < alternatives.size(); ++i) {
        for (size_t j = i + 1; j < alternatives.size(); ++j) {
            auto prefix = findCommonPrefix(alternatives[i], alternatives[j]);
            if (prefix) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::vector<const RETree*>> LeftFactorization::groupByPrefix(
    const std::vector<const RETree*>& alternatives
) {
    // НОВЫЙ АЛГОРИТМ:
    // 1. Находим все пары с общими префиксами
    // 2. Группируем пары по одинаковому префиксу
    // 3. Объединяем пересекающиеся группы
    
    std::map<std::string, std::vector<const RETree*>> prefixMap;
    std::set<const RETree*> processed;
    
    for (size_t i = 0; i < alternatives.size(); ++i) {
        for (size_t j = i + 1; j < alternatives.size(); ++j) {
            auto prefix = findCommonPrefix(alternatives[i], alternatives[j]);
            if (prefix) {
                SelectionMask emptyMask;
                std::string prefixStr = prefix->toString(emptyMask, false);
                
                auto& group = prefixMap[prefixStr];
                
                // Добавляем обе альтернативы, если их ещё нет
                if (std::find(group.begin(), group.end(), alternatives[i]) == group.end()) {
                    group.push_back(alternatives[i]);
                }
                if (std::find(group.begin(), group.end(), alternatives[j]) == group.end()) {
                    group.push_back(alternatives[j]);
                }
                
                processed.insert(alternatives[i]);
                processed.insert(alternatives[j]);
            }
        }
    }
    
    // Формируем итоговые группы
    std::vector<std::vector<const RETree*>> groups;
    
    // Сначала добавляем группы с общими префиксами
    for (auto& [prefixStr, group] : prefixMap) {
        // Проверяем, что все элементы группы действительно имеют этот префикс
        if (group.size() > 1) {
            groups.push_back(std::move(group));
        }
    }
    
    // Затем добавляем одиночные альтернативы
    for (const auto* alt : alternatives) {
        if (processed.find(alt) == processed.end()) {
            groups.push_back({alt});
        }
    }
    
    return groups;
}

std::unique_ptr<RETree> LeftFactorization::buildFactorizedRule(
    const std::vector<std::vector<const RETree*>>& groups,
    Grammar* grammar
) {
    if (groups.empty()) return nullptr;
    
    std::unique_ptr<RETree> result;
    
    for (const auto& group : groups) {
        if (group.size() == 1) {
            // Одиночная альтернатива - просто копируем
            auto alt = group[0]->copy();
            
            if (!result) {
                result = std::move(alt);
            } else {
                result = REOr::make(std::move(result), std::move(alt));
            }
        } else if (group.size() > 1) {
            // Группа с общим префиксом - факторизуем
            
            // Находим общий префикс между первыми двумя элементами группы
            auto prefix = findCommonPrefix(group[0], group[1]);
            if (!prefix) continue;
            
            // Создаем новый нетерминал для суффиксов
            std::string newName = "factored_" + std::to_string(grammar->getNonTerminals().size());
            grammar->addNonTerminal(newName);
            
            // Строим правило для нового нетерминала (суффиксы)
            std::unique_ptr<RETree> suffixRule;
            for (const auto* tree : group) {
                auto suffix = removePrefix(tree, prefix.get());
                if (!suffix) {
                    // Если суффикс пустой - добавляем epsilon
                    int epsilonId = grammar->addSemantic("@");
                    suffix = std::make_unique<RESemantic>(grammar, epsilonId);
                }
                
                if (!suffixRule) {
                    suffixRule = std::move(suffix);
                } else {
                    suffixRule = REOr::make(std::move(suffixRule), std::move(suffix));
                }
            }
            
            // Сохраняем правило для нового нетерминала
            if (suffixRule) {
                grammar->setNTRoot(newName, std::move(suffixRule));
            }
            
            // // Рекурсивно факторизуем новый нетерминал
            // NTListItem* newNT = grammar->getNTItem(newName);
            // if (newNT) {
            //     factorize(newNT, grammar);
            // }
            
            // Строим префикс + новый нетерминал
            int newNTId = grammar->findNonTerminal(newName);
            auto newNTNode = std::make_unique<RENonTerminal>(grammar, newNTId, false);
            auto factored = REAnd::make(std::move(prefix), std::move(newNTNode));
            
            if (!result) {
                result = std::move(factored);
            } else {
                result = REOr::make(std::move(result), std::move(factored));
            }
        }
    }
    
    return result;
}

std::unique_ptr<RETree> LeftFactorization::removePrefix(
    const RETree* tree,
    const RETree* prefix
) {
    if (!tree || !prefix) return nullptr;
    
    // Развернуть оба в списки
    std::vector<const RETree*> treeList, prefixList;
    flattenAndChain(tree, treeList);
    flattenAndChain(prefix, prefixList);
    
    // Проверяем что prefix действительно префикс
    if (prefixList.size() > treeList.size()) return tree->copy();
    
    for (size_t i = 0; i < prefixList.size(); ++i) {
        if (!treesEqual(treeList[i], prefixList[i])) {
            return tree->copy();  // Не префикс
        }
    }
    
    // Если префикс == всё дерево, возвращаем nullptr (пустой суффикс)
    if (prefixList.size() == treeList.size()) {
        return nullptr;
    }
    
    // Собираем суффикс
    return buildAndChain(treeList, prefixList.size(), treeList.size());
}

bool LeftFactorization::treesEqual(const RETree* tree1, const RETree* tree2) {
    if (!tree1 && !tree2) return true;
    if (!tree1 || !tree2) return false;
    
    SelectionMask emptyMask;
    return tree1->toString(emptyMask, false) == tree2->toString(emptyMask, false);
}

}