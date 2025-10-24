#include <syngt/utils/Semantic.h>
#include <iostream>
#include <sstream>

namespace syngt {

void SemanticIDList::save() {
    std::cout << count() << "\n";
    for (int i = 0; i < count(); ++i) {
        std::cout << getID(i) << "\n";
    }
}

void SemanticIDList::load() {
    int idCount;
    std::cin >> idCount;
    
    m_items.clear();
    m_items.reserve(idCount);
    
    for (int i = 0; i < idCount; ++i) {
        int curID;
        std::cin >> curID;
        add(curID);
    }
}

}