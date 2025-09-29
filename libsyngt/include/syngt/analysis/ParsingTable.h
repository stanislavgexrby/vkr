#pragma once
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <set>

namespace syngt {

class Grammar;
class RETree;

/**
 * @brief Таблица разбора LL(1)
 * 
 * M[A, a] определяет какое правило использовать
 * когда нетерминал A на вершине стека и терминал a на входе
 */
class ParsingTable {
public:
    // Ключ таблицы: (нетерминал, терминал)
    using TableKey = std::pair<std::string, int>;
    
    // Значение: правило (дерево)
    using TableValue = const RETree*;
    
    /**
     * @brief Построить таблицу разбора для грамматики
     */
    static std::unique_ptr<ParsingTable> build(Grammar* grammar);
    
    /**
     * @brief Получить правило из таблицы
     * @return nullptr если ячейка пустая
     */
    const RETree* getRule(const std::string& nonTerminal, int terminal) const;
    
    /**
     * @brief Проверить заполнена ли ячейка
     */
    bool hasRule(const std::string& nonTerminal, int terminal) const;
    
    /**
     * @brief Проверить на конфликты (несколько правил в одной ячейке)
     */
    bool hasConflicts() const { return !m_conflicts.empty(); }
    
    /**
     * @brief Получить список конфликтов
     */
    const std::vector<std::string>& getConflicts() const { return m_conflicts; }
    
    /**
     * @brief Вывести таблицу в читаемом виде
     */
    void print(Grammar* grammar) const;
    
    /**
     * @brief Экспортировать таблицу в формат для кодогенерации
     */
    std::string exportForCodegen(Grammar* grammar) const;
    
private:
    std::map<TableKey, TableValue> m_table;
    std::vector<std::string> m_conflicts;
    Grammar* m_grammar = nullptr;
    
    ParsingTable() = default;
    
    /**
     * @brief Добавить правило в таблицу (с проверкой конфликтов)
     */
    void addRule(const std::string& nonTerminal, int terminal, const RETree* rule);
    
    /**
     * @brief Обработать одну альтернативу правила
     */
    void processAlternative(
        const std::string& ntName,
        const RETree* alternative,
        const std::map<std::string, std::set<int>>& firstSets,
        const std::map<std::string, std::set<int>>& followSets,
        const std::map<std::string, bool>& nullable
    );
};

}