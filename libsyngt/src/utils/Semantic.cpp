#include <syngt/utils/Semantic.h>
#include <iostream>

namespace syngt {

void SemanticIDList::save() {
    std::cout << m_items.size() << std::endl;
    for (int id : m_items) {
        std::cout << id << std::endl;
    }
}

void SemanticIDList::load() {
    int count;
    std::cin >> count;
    
    m_items.clear();
    m_items.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        int id;
        std::cin >> id;
        m_items.push_back(id);
    }
}

}