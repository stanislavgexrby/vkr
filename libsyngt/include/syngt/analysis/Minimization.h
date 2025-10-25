#pragma once
#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>

/**
 * @brief Минимизация конечных автоматов
 * 
 * Построение и минимизация детерминированных конечных автоматов (DFA)
 * для регулярных выражений грамматик.
 * 
 * Соответствует Pascal: Minimization.pas
 */

namespace syngt {

// Forward declarations
class Grammar;

/**
 * @brief Тип состояния автомата
 */
using State = int;

/**
 * @brief Специальные состояния
 */
constexpr State StartState = 0;
constexpr State FinalState = 1;

/**
 * @brief Символ пустого перехода (epsilon)
 */
constexpr const char* EmptySymbol = "";

/**
 * @brief Набор состояний
 * 
 * Используется для хранения множества состояний в ячейках таблицы.
 * Соответствует Pascal: TStatesSet
 */
class StatesSet {
private:
    std::set<State> m_states;
    
public:
    StatesSet() = default;
    
    /**
     * @brief Добавить состояние в набор
     */
    void addState(State state) {
        m_states.insert(state);
    }
    
    /**
     * @brief Проверить наличие состояния
     */
    bool findState(State state) const {
        return m_states.find(state) != m_states.end();
    }
    
    /**
     * @brief Получить все состояния
     */
    const std::set<State>& getStates() const {
        return m_states;
    }
    
    /**
     * @brief Количество состояний
     */
    size_t count() const {
        return m_states.size();
    }
    
    /**
     * @brief Пустой ли набор
     */
    bool empty() const {
        return m_states.empty();
    }
};

/**
 * @brief Запись для построения таблицы минимизации
 * 
 * Хранит начальное и конечное состояние для текущего фрагмента автомата.
 * Соответствует Pascal: TMinRecord
 */
struct MinRecord {
    State start;   // Начальное состояние
    State finish;  // Конечное состояние
    
    MinRecord() : start(StartState), finish(FinalState) {}
    MinRecord(State s, State f) : start(s), finish(f) {}
};

/**
 * @brief Таблица минимизации автомата
 * 
 * Представляет таблицу переходов конечного автомата.
 * Строки - состояния, столбцы - символы входного алфавита.
 * 
 * Алгоритм:
 * 1. Построение NFA из регулярного выражения
 * 2. Детерминизация (NFA → DFA)
 * 3. Минимизация (удаление эквивалентных состояний)
 * 
 * Соответствует Pascal: TMinimizationTable
 */
class MinimizationTable {
private:
    // Таблица переходов: [состояние][символ] → набор состояний
    std::map<std::pair<State, std::string>, std::unique_ptr<StatesSet>> m_table;
    
    // Список символов алфавита
    std::vector<std::string> m_symbols;
    
    // Имена состояний (StartState, FinalState, State2, State3, ...)
    std::map<State, std::string> m_stateNames;
    
    // Счетчик состояний
    State m_nextState = 2; // 0=Start, 1=Final, 2+ = промежуточные
    
    /**
     * @brief Найти или добавить символ в алфавит
     */
    int findOrAddSymbol(const std::string& symbol);
    
    /**
     * @brief Детерминизация автомата (NFA → DFA)
     */
    void determinize();
    
    /**
     * @brief Минимизация методом разбиения на классы эквивалентности
     */
    void minimizeByEquivalence();
    
public:
    MinimizationTable() = default;
    ~MinimizationTable() = default;
    
    /**
     * @brief Создать новое состояние
     * @return ID нового состояния
     */
    State createState();
    
    /**
     * @brief Связать два состояния переходом по символу
     * 
     * @param from Начальное состояние
     * @param to Конечное состояние
     * @param symbol Символ перехода (или EmptySymbol для epsilon)
     */
    void linkStates(State from, State to, const std::string& symbol);
    
    /**
     * @brief Получить элемент таблицы (набор состояний)
     * 
     * @return Набор состояний или nullptr если переход не определен
     */
    const StatesSet* getTableElement(State state, int symbolIndex) const;
    
    /**
     * @brief Получить символ по индексу
     */
    const std::string& getSymbol(int index) const;
    
    /**
     * @brief Получить имя состояния
     */
    const std::string& getStateName(State state) const;
    
    /**
     * @brief Установить имя состояния
     */
    void setStateName(State state, const std::string& name);
    
    /**
     * @brief Количество состояний
     */
    int getStatesCount() const {
        return static_cast<int>(m_nextState);
    }
    
    /**
     * @brief Количество символов
     */
    int getSymbolsCount() const {
        return static_cast<int>(m_symbols.size());
    }
    
    /**
     * @brief Выполнить минимизацию автомата
     * 
     * Применяет детерминизацию и минимизацию.
     */
    void minimize();
    
    /**
     * @brief Записать таблицу в файл (для отладки)
     */
    void writeToFile(const std::string& filename) const;
    
    /**
     * @brief Вывести таблицу в строку
     */
    std::string toString() const;
};

} // namespace syngt