#pragma once
#include <set>
#include <map>
#include <string>
#include <vector>

namespace syngt {

class Grammar;
class NTListItem;
class RETree;

/**
 * @brief FIRST и FOLLOW множества для LL(1) анализа
 * 
 * FIRST(α) - множество терминалов, с которых может начинаться вывод из α
 * FOLLOW(A) - множество терминалов, которые могут следовать за A
 */
class FirstFollow {
public:
    using TerminalSet = std::set<int>;  // Множество индексов терминалов
    
    /**
     * @brief Вычислить FIRST множества для всех нетерминалов
     */
    static std::map<std::string, TerminalSet> computeFirst(Grammar* grammar);
    
    /**
     * @brief Вычислить FOLLOW множества для всех нетерминалов
     */
    static std::map<std::string, TerminalSet> computeFollow(
        Grammar* grammar,
        const std::map<std::string, TerminalSet>& firstSets
    );
    
    /**
     * @brief Проверить является ли грамматика LL(1)
     * @return true если грамматика LL(1)
     */
    static bool isLL1(Grammar* grammar);
    
    /**
     * @brief Вывести FIRST/FOLLOW множества в читаемом виде
     */
    static void printSets(
        Grammar* grammar,
        const std::map<std::string, TerminalSet>& firstSets,
        const std::map<std::string, TerminalSet>& followSets
    );
    
private:
    /**
     * @brief Вычислить FIRST для дерева выражения
     */
    static TerminalSet computeFirstForTree(
        const RETree* tree,
        const std::map<std::string, TerminalSet>& knownFirst,
        bool& nullable
    );
    
    /**
     * @brief Проверить может ли дерево выводить пустую строку (epsilon)
     */
    static bool isNullable(
        const RETree* tree,
        const std::map<std::string, bool>& knownNullable
    );
    
    /**
     * @brief Проверить LL(1) условие для одного нетерминала
     */
    static bool checkLL1ForNT(
        NTListItem* nt,
        const std::map<std::string, TerminalSet>& firstSets,
        const std::map<std::string, TerminalSet>& followSets
    );
    
    /**
     * @brief Собрать все альтернативы из Or-дерева
     */
    static void collectAlternatives(
        const RETree* tree,
        std::vector<const RETree*>& alternatives
    );
};

}