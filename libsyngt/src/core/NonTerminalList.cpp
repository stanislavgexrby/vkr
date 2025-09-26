#include <syngt/core/NonTerminalList.h>
#include <algorithm>
#include <stdexcept>

namespace syngt {

int NonTerminalList::add(const std::string& s) {
    auto it = std::find(m_items.begin(), m_items.end(), s);
    
    if (it != m_items.end()) {
        return static_cast<int>(std::distance(m_items.begin(), it));
    }
    
    m_items.push_back(s);
    return static_cast<int>(m_items.size() - 1);
}

int NonTerminalList::find(const std::string& s) const {
    auto it = std::find(m_items.begin(), m_items.end(), s);
    
    if (it != m_items.end()) {
        return static_cast<int>(std::distance(m_items.begin(), it));
    }
    
    return -1;
}

std::string NonTerminalList::getString(int index) const {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        throw std::out_of_range("NonTerminalList: index out of range");
    }
    
    return m_items[index];
}

void NonTerminalList::fillNew() {
    m_items.clear();
    add("S");
}

}