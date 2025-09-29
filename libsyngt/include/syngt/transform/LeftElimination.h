#pragma once
#include <memory>
#include <vector>

namespace syngt {

class Grammar;
class NTListItem;
class RETree;

/**
 * @brief Устранение левой рекурсии в грамматике
 * 
 * Трансформирует правила вида:
 *   A → A α | β
 * В правила:
 *   A → β A'
 *   A' → α A' | ε
 */
class LeftElimination {
public:
    /**
     * @brief Устранить левую рекурсию для одного нетерминала
     */
    static void eliminateForNonTerminal(NTListItem* nt, Grammar* grammar);
    
    /**
     * @brief Устранить левую рекурсию для всей грамматики
     */
    static void eliminate(Grammar* grammar);
    
    /**
     * @brief Проверка на прямую левую рекурсию (A → A α)
     */
    static bool hasDirectLeftRecursion(NTListItem* nt);
private:    
    /**
     * @brief Проверка узла на левую рекурсию
     */
    static bool isLeftRecursive(const RETree* node, const NTListItem* nt);
    
    /**
     * @brief Разделить правило на α (рекурсивные) и β (нерекурсивные) части
     */
    static void collectAlphaAndBeta(
        const RETree* root,
        const NTListItem* nt,
        std::vector<std::unique_ptr<RETree>>& alphaList,
        std::vector<std::unique_ptr<RETree>>& betaList
    );
    
    /**
     * @brief Извлечь α из "A α" (убирает A)
     */
    static std::unique_ptr<RETree> extractAlpha(const RETree* node, const NTListItem* nt);
    
    /**
     * @brief Построить A → β A'
     */
    static std::unique_ptr<RETree> buildBetaWithRecursion(
        std::vector<std::unique_ptr<RETree>>& betaList,
        const std::string& recursiveName,
        Grammar* grammar
    );
    
    /**
     * @brief Построить A' → α A' | ε
     */
    static std::unique_ptr<RETree> buildAlphaWithRecursion(
        std::vector<std::unique_ptr<RETree>>& alphaList,
        const std::string& recursiveName,
        Grammar* grammar
    );
};

}