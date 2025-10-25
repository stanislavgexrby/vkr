#include <syngt/core/TerminalList.h>
#include <algorithm>
#include <stdexcept>

namespace syngt {

int TerminalList::add(const std::string& s) {
    auto it = std::find(m_items.begin(), m_items.end(), s);
    
    if (it != m_items.end()) {
        return static_cast<int>(std::distance(m_items.begin(), it));
    }
    
    m_items.push_back(s);
    return static_cast<int>(m_items.size() - 1);
}

int TerminalList::find(const std::string& s) const {
    auto it = std::find(m_items.begin(), m_items.end(), s);
    
    if (it != m_items.end()) {
        return static_cast<int>(std::distance(m_items.begin(), it));
    }
    
    return -1;
}

std::string TerminalList::getString(int index) const {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        throw std::out_of_range("TerminalList: index out of range");
    }
    
    // Специальная обработка epsilon (пустой строки)
    // Epsilon всегда имеет ID=0 и отображается как @
    if (index == 0 && m_items[0].empty()) {
        return "@";
    }
    
    return m_items[index];
}

}