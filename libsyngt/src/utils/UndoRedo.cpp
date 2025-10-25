#include <syngt/utils/UndoRedo.h>
#include <algorithm>

namespace syngt {

UndoRedo::~UndoRedo() {
    clearData();
}

bool UndoRedo::equalState(const UndoState* s1, const UndoState* s2) const {
    if (!s1 || !s2) {
        return false;
    }
    
    // Проверяем размеры
    if (s1->ntNames.size() != s2->ntNames.size() ||
        s1->ntValues.size() != s2->ntValues.size() ||
        s1->ntNames.size() != s1->ntValues.size()) {
        return false;
    }
    
    // Сравниваем имена и значения
    for (size_t i = 0; i < s1->ntNames.size(); ++i) {
        if (s1->ntNames[i] != s2->ntNames[i] ||
            s1->ntValues[i] != s2->ntValues[i]) {
            return false;
        }
    }
    
    return true;
}

void UndoRedo::freeForward(UndoState* from) {
    if (!from) return;
    
    UndoState* curr = from;
    while (curr) {
        UndoState* next = curr->next;
        delete curr;
        curr = next;
    }
}

void UndoRedo::addState(const std::vector<std::string>& ntNames,
                        const std::vector<std::string>& ntValues,
                        int activeIndex,
                        const SelectionMask& selection) {
    
    UndoState* next = m_current ? m_current->next : nullptr;
    
    // Создаем новое состояние
    auto newState = new UndoState();
    newState->ntNames = ntNames;
    newState->ntValues = ntValues;
    newState->activeIndex = activeIndex;
    
    // Обновляем selection у текущего состояния (если оно есть)
    if (m_current && !selection.empty()) {
        m_current->selection = selection;
    }
    
    // Проверка на дубликаты
    if (equalState(m_current, newState) || equalState(next, newState)) {
        // Одинаковое состояние - не добавляем
        delete newState;
        return;
    }
    
    // Связываем с текущим
    newState->prev = m_current;
    newState->next = nullptr;
    
    if (m_current) {
        // Удаляем всю цепочку вперед от текущего
        if (m_current->next) {
            freeForward(m_current->next);
        }
        m_current->next = newState;
    } else {
        // Первое состояние
        m_begin = newState;
    }
    
    // Обновляем указатели
    m_current = newState;
    m_last = newState;
}

bool UndoRedo::stepBack(std::vector<std::string>& outNames,
                        std::vector<std::string>& outValues,
                        int& outActiveIndex,
                        SelectionMask& outSelection) {
    
    if (!canUndo()) {
        return false;
    }
    
    // Переходим к предыдущему состоянию
    m_current = m_current->prev;
    
    // Копируем данные
    outNames = m_current->ntNames;
    outValues = m_current->ntValues;
    outActiveIndex = m_current->activeIndex;
    outSelection = m_current->selection;
    
    return true;
}

bool UndoRedo::stepForward(std::vector<std::string>& outNames,
                           std::vector<std::string>& outValues,
                           int& outActiveIndex,
                           SelectionMask& outSelection) {
    
    if (!canRedo()) {
        return false;
    }
    
    // Переходим к следующему состоянию
    m_current = m_current->next;
    
    // Копируем данные
    outNames = m_current->ntNames;
    outValues = m_current->ntValues;
    outActiveIndex = m_current->activeIndex;
    outSelection = m_current->selection;
    
    return true;
}

void UndoRedo::clearData() {
    // Освобождаем всю цепочку
    freeForward(m_begin);
    
    m_begin = nullptr;
    m_current = nullptr;
    m_last = nullptr;
}

}