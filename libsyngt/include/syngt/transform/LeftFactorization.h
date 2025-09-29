#pragma once
#include <vector>
#include <memory>

namespace syngt {

class Grammar;
class NTListItem;
class RETree;

/**
 * @brief Левая факторизация грамматики
 * 
 * Устраняет общие префиксы в альтернативах:
 *   A → α β | α γ
 * Преобразует в:
 *   A → α A'
 *   A' → β | γ
 */
class LeftFactorization {
public:
    /**
     * @brief Факторизовать один нетерминал
     */
    static void factorize(NTListItem* nt, Grammar* grammar);
    
    /**
     * @brief Факторизовать всю грамматику
     */
    static void factorizeAll(Grammar* grammar);
    
private:
    /**
     * @brief Собрать все альтернативы из Or-узлов
     */
    static void collectAlternatives(
        const RETree* root,
        std::vector<const RETree*>& alternatives
    );
    
    /**
     * @brief Найти общий префикс двух деревьев
     * @return nullptr если общего префикса нет
     */
    static std::unique_ptr<RETree> findCommonPrefix(
        const RETree* tree1,
        const RETree* tree2
    );
    
    /**
     * @brief Проверить, есть ли общие префиксы в списке альтернатив
     */
    static bool hasCommonPrefixes(
        const std::vector<const RETree*>& alternatives
    );
    
    /**
     * @brief Сгруппировать альтернативы по общему префиксу
     */
    static std::vector<std::vector<const RETree*>> groupByPrefix(
        const std::vector<const RETree*>& alternatives
    );
    
    /**
     * @brief Построить факторизованное правило
     */
    static std::unique_ptr<RETree> buildFactorizedRule(
        const std::vector<std::vector<const RETree*>>& groups,
        Grammar* grammar
    );
    
    /**
     * @brief Убрать префикс из дерева
     */
    static std::unique_ptr<RETree> removePrefix(
        const RETree* tree,
        const RETree* prefix
    );
    
    /**
     * @brief Сравнить два дерева на равенство
     */
    static bool treesEqual(const RETree* tree1, const RETree* tree2);
};

}