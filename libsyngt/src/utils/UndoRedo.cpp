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
    
    if (s1->ntNames.size() != s2->ntNames.size() ||
        s1->ntValues.size() != s2->ntValues.size() ||
        s1->ntNames.size() != s1->ntValues.size()) {
        return false;
    }
    
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
    
    auto newState = new UndoState();
    newState->ntNames = ntNames;
    newState->ntValues = ntValues;
    newState->activeIndex = activeIndex;
    
    if (m_current && !selection.empty()) {
        m_current->selection = selection;
    }
    
    if (equalState(m_current, newState) || equalState(next, newState)) {
        delete newState;
        return;
    }
    
    newState->prev = m_current;
    newState->next = nullptr;
    
    if (m_current) {
        if (m_current->next) {
            freeForward(m_current->next);
        }
        m_current->next = newState;
    } else {
        m_begin = newState;
    }
    
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
    
    m_current = m_current->prev;
    
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
    
    m_current = m_current->next;
    
    outNames = m_current->ntNames;
    outValues = m_current->ntValues;
    outActiveIndex = m_current->activeIndex;
    outSelection = m_current->selection;
    
    return true;
}

void UndoRedo::clearData() {
    freeForward(m_begin);
    
    m_begin = nullptr;
    m_current = nullptr;
    m_last = nullptr;
}

}