#pragma once
#include <string>
#include <vector>
#include <memory>

namespace syngt {

class Grammar;

/**
 * @brief Маска выделения (ID объектов)
 * 
 * Соответствует Pascal: TSelMas = array of integer
 */
using SelectionMask = std::vector<int>;

/**
 * @brief Состояние для Undo/Redo
 * 
 * Хранит снимок состояния грамматики: имена нетерминалов,
 * их значения, индекс активного нетерминала и маску выделения.
 * 
 * Соответствует Pascal: TUndoState
 */
struct UndoState {
    std::vector<std::string> ntNames;     // Имена нетерминалов
    std::vector<std::string> ntValues;    // Определения нетерминалов
    int activeIndex = 0;                  // Индекс активного нетерминала
    SelectionMask selection;              // Маска выделенных объектов
    
    UndoState* prev = nullptr;
    UndoState* next = nullptr;
    
    UndoState() = default;
    ~UndoState() = default;
};

/**
 * @brief Система Undo/Redo для грамматики
 * 
 * Двусвязный список состояний. Поддерживает:
 * - Сохранение состояния грамматики
 * - Откат к предыдущему состоянию (Undo)
 * - Повтор отмененного действия (Redo)
 * - Оптимизация: не сохраняет идентичные состояния подряд
 * 
 * Соответствует Pascal: TUndoRedo
 */
class UndoRedo {
private:
    UndoState* m_begin = nullptr;   // Начало списка
    UndoState* m_current = nullptr; // Текущее состояние
    UndoState* m_last = nullptr;    // Конец списка
    
    /**
     * @brief Сравнить два состояния на равенство
     * @return true если состояния идентичны
     */
    bool equalState(const UndoState* s1, const UndoState* s2) const;
    
    /**
     * @brief Освободить память от узла вперед
     */
    void freeForward(UndoState* from);
    
public:
    UndoRedo() = default;
    ~UndoRedo();
    
    /**
     * @brief Добавить новое состояние
     * 
     * Создает снимок текущего состояния грамматики.
     * Не добавляет если новое состояние идентично текущему или следующему.
     * 
     * @param ntNames Имена нетерминалов
     * @param ntValues Определения нетерминалов
     * @param activeIndex Индекс активного нетерминала
     * @param selection Маска выделения
     */
    void addState(const std::vector<std::string>& ntNames,
                  const std::vector<std::string>& ntValues,
                  int activeIndex,
                  const SelectionMask& selection);
    
    /**
     * @brief Шаг назад (Undo)
     * 
     * @param outNames Возвращает имена нетерминалов предыдущего состояния
     * @param outValues Возвращает определения предыдущего состояния
     * @param outActiveIndex Возвращает индекс активного нетерминала
     * @param outSelection Возвращает маску выделения
     * @return true если откат успешен, false если достигнуто начало
     */
    bool stepBack(std::vector<std::string>& outNames,
                  std::vector<std::string>& outValues,
                  int& outActiveIndex,
                  SelectionMask& outSelection);
    
    /**
     * @brief Шаг вперед (Redo)
     * 
     * @param outNames Возвращает имена нетерминалов следующего состояния
     * @param outValues Возвращает определения следующего состояния
     * @param outActiveIndex Возвращает индекс активного нетерминала
     * @param outSelection Возвращает маску выделения
     * @return true если повтор успешен, false если достигнут конец
     */
    bool stepForward(std::vector<std::string>& outNames,
                     std::vector<std::string>& outValues,
                     int& outActiveIndex,
                     SelectionMask& outSelection);
    
    /**
     * @brief Очистить все данные
     */
    void clearData();
    
    /**
     * @brief Проверить возможность Undo
     */
    bool canUndo() const { return m_current != nullptr && m_current->prev != nullptr; }
    
    /**
     * @brief Проверить возможность Redo
     */
    bool canRedo() const { return m_current != nullptr && m_current->next != nullptr; }
};

}