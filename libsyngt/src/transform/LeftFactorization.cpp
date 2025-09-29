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
    
    // Повторяем факторизацию пока есть изменения (как в Pascal)
    const int maxIterations = 10;  // Защита от бесконечного цикла
    
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        RETree* root = nt->root();
        if (!root) return;
        
        // Собираем все альтернативы
        std::vector<const RETree*> alternatives;
        collectAlternatives(root, alternatives);
        
        // Если альтернатив меньше 2 - факторизовать нечего
        if (alternatives.size() < 2) return;
        
        // Проверяем наличие общих префиксов
        if (!hasCommonPrefixes(alternatives)) {
            return;  // Нет общих префиксов - выходим
        }
        
        // Группируем по общим префиксам
        auto groups = groupByPrefix(alternatives);
        
        // Если группировка не дала результата - выходим
        if (groups.size() == alternatives.size()) {
            return;  // Каждая альтернатива в своей группе - нет факторизации
        }
        
        // Строим новое правило
        auto newRule = buildFactorizedRule(groups, grammar);
        if (!newRule) return;
        
        nt->setRoot(std::move(newRule));
        
        // Продолжаем следующую итерацию для рекурсивной факторизации
    }
}

void LeftFactorization::collectAlternatives(
    const RETree* root,
    std::vector<const RETree*>& alternatives
) {
    if (!root) return;
    
    // Если это Or - рекурсивно собираем альтернативы
    if (auto orNode = dynamic_cast<const REOr*>(root)) {
        collectAlternatives(orNode->left(), alternatives);
        collectAlternatives(orNode->right(), alternatives);
    } else {
        // Это одиночная альтернатива
        alternatives.push_back(root);
    }
}

std::unique_ptr<RETree> LeftFactorization::findCommonPrefix(
    const RETree* tree1,
    const RETree* tree2
) {
    if (!tree1 || !tree2) return nullptr;
    
    // Если оба - And узлы, проверяем первые элементы
    auto and1 = dynamic_cast<const REAnd*>(tree1);
    auto and2 = dynamic_cast<const REAnd*>(tree2);
    
    if (and1 && and2) {
        // Сравниваем левые части
        if (treesEqual(and1->left(), and2->left())) {
            // Есть общий префикс - возвращаем его
            return and1->left()->copy();
        }
    } else if (!and1 && !and2) {
        // Оба - простые узлы, сравниваем напрямую
        if (treesEqual(tree1, tree2)) {
            return tree1->copy();
        }
    }
    
    return nullptr;
}

bool LeftFactorization::hasCommonPrefixes(
    const std::vector<const RETree*>& alternatives
) {
    // Проверяем каждую пару альтернатив
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
    std::vector<std::vector<const RETree*>> groups;
    std::vector<bool> used(alternatives.size(), false);
    
    // Группируем альтернативы с одинаковыми префиксами
    for (size_t i = 0; i < alternatives.size(); ++i) {
        if (used[i]) continue;
        
        std::vector<const RETree*> group;
        group.push_back(alternatives[i]);
        used[i] = true;
        
        // Ищем другие альтернативы с таким же префиксом
        for (size_t j = i + 1; j < alternatives.size(); ++j) {
            if (used[j]) continue;
            
            auto prefix = findCommonPrefix(alternatives[i], alternatives[j]);
            if (prefix) {
                group.push_back(alternatives[j]);
                used[j] = true;
            }
        }
        
        groups.push_back(group);
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
            
            // Находим общий префикс (между первыми двумя)
            auto prefix = findCommonPrefix(group[0], group[1]);
            if (!prefix) continue;  // Не должно случиться, но защита
            
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
            
            // Рекурсивно факторизуем новый нетерминал
            NTListItem* newNT = grammar->getNTItem(newName);
            if (newNT) {
                factorize(newNT, grammar);  // ← Рекурсивный вызов!
            }
            
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
    
    // Если дерево - And и префикс совпадает с левой частью
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        if (treesEqual(andNode->left(), prefix)) {
            // Возвращаем правую часть (суффикс)
            return andNode->right()->copy();
        }
    }
    
    if (treesEqual(tree, prefix)) {
        return nullptr;
    }
    
    return tree->copy();
}

bool LeftFactorization::treesEqual(const RETree* tree1, const RETree* tree2) {
    if (!tree1 && !tree2) return true;
    if (!tree1 || !tree2) return false;
    
    SelectionMask emptyMask;
    return tree1->toString(emptyMask, false) == tree2->toString(emptyMask, false);
}

}