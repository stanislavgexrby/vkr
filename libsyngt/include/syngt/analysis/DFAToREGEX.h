#pragma once
#include <syngt/analysis/Minimization.h>
#include <vector>
#include <memory>

/**
 * @brief Преобразование конечного автомата в регулярное выражение
 * 
 * Реализует алгоритм Ардена для преобразования DFA в регулярное выражение
 * путем последовательного удаления состояний.
 * 
 * Соответствует Pascal: DugaAutomat.pas
 */

namespace syngt {

// Forward declarations
class Grammar;
class RETree;

/**
 * @brief Дуга (переход) автомата
 * 
 * Представляет переход между двумя состояниями с регулярным выражением.
 * Соответствует Pascal: TDuga
 */
struct Arc {
    State fromState;               // Начальное состояние
    State toState;                 // Конечное состояние
    std::unique_ptr<RETree> tree;  // Регулярное выражение перехода
    
    Arc(State from, State to, std::unique_ptr<RETree> t)
        : fromState(from), toState(to), tree(std::move(t)) {}
};

/**
 * @brief Преобразователь автомата в регулярное выражение
 * 
 * Алгоритм:
 * 1. Строим граф дуг из таблицы минимизации
 * 2. Объединяем параллельные дуги (A→B, A→B) через OR
 * 3. Удаляем промежуточные состояния одно за другим:
 *    - Для каждой пары входящих/исходящих дуг создаем новую дугу
 *    - Учитываем петли (циклы) как итерации
 * 4. В конце остается одна дуга Start→Final с итоговым RE
 * 
 * Соответствует Pascal: TDugaAutomat
 */
class DFAToRegex {
private:
    Grammar* m_grammar;
    std::vector<std::unique_ptr<Arc>> m_arcs;  // Список дуг
    std::vector<State> m_states;               // Список состояний
    
    /**
     * @brief Объединить все дуги с одинаковыми from/to через OR
     */
    void mergeParallelArcs();
    
    /**
     * @brief Удалить состояние из автомата
     * 
     * Создает новые дуги, обходящие удаляемое состояние:
     * Для каждой пары (X→S, S→Y) создается дуга X→Y
     * с выражением: X→S, (S→S)*, S→Y
     * 
     * @param stateIndex Индекс состояния в m_states
     */
    void removeState(size_t stateIndex);
    
    /**
     * @brief Найти оптимальное состояние для удаления
     * 
     * Выбирает состояние с минимальной "стоимостью" удаления.
     * Стоимость = количество новых дуг, которые будут созданы.
     * 
     * @return Индекс оптимального состояния
     */
    size_t findBestStateToRemove();
    
    /**
     * @brief Вычислить стоимость удаления состояния
     * 
     * Стоимость = inCount * outCount * roundLength + 
     *             inLength * (outCount-1) + 
     *             outLength * (inCount-1)
     * 
     * Где:
     * - inCount: количество входящих дуг
     * - outCount: количество исходящих дуг  
     * - roundLength: длина петли (если есть)
     * 
     * @return Стоимость или -1 если нужен перерасчет
     */
    int getStateRemovalCost(State state);
    
    /**
     * @brief Проверить и оптимизировать бинарную итерацию
     * 
     * Если состояние имеет:
     * - Ровно 1 входящую дугу (A→S)
     * - Ровно 1 исходящую дугу (S→B)
     * - Обратную дугу (B→S)
     * - Нет петли на S
     * 
     * То создается итерация: A→S, (S→B, B→S)*, S→B
     * 
     * @return true если оптимизация применена
     */
    bool tryOptimizeBinaryIteration(Arc* arc);
    
    /**
     * @brief Создать epsilon терминал
     */
    std::unique_ptr<RETree> createEpsilon();
    
    /**
     * @brief Создать унарную итерацию (E)*
     */
    std::unique_ptr<RETree> createUnaryIteration(std::unique_ptr<RETree> tree);
    
    /**
     * @brief Создать последовательность E1, E2
     */
    std::unique_ptr<RETree> createAnd(
        std::unique_ptr<RETree> left, 
        std::unique_ptr<RETree> right
    );
    
    /**
     * @brief Создать альтернативу E1 ; E2
     */
    std::unique_ptr<RETree> createOr(
        std::unique_ptr<RETree> left,
        std::unique_ptr<RETree> right
    );
    
    /**
     * @brief Создать бинарную итерацию E1 # E2
     */
    std::unique_ptr<RETree> createIteration(
        std::unique_ptr<RETree> left,
        std::unique_ptr<RETree> right
    );
    
public:
    /**
     * @brief Создать преобразователь из таблицы минимизации
     * 
     * @param grammar Грамматика для создания RE
     * @param table Таблица минимизированного автомата
     */
    static std::unique_ptr<DFAToRegex> fromMinimizationTable(
        Grammar* grammar,
        const MinimizationTable* table
    );
    
    /**
     * @brief Удалить все промежуточные состояния
     * 
     * Последовательно удаляет состояния, пока не останется
     * только Start→Final с итоговым регулярным выражением.
     */
    void removeAllStates();
    
    /**
     * @brief Получить итоговое регулярное выражение
     * 
     * Должно вызываться после removeAllStates().
     * 
     * @return Регулярное выражение автомата
     */
    std::unique_ptr<RETree> getRegularExpression();
    
    DFAToRegex(Grammar* grammar) : m_grammar(grammar) {}
    ~DFAToRegex() = default;
};

} // namespace syngt