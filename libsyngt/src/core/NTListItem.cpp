#include <syngt/core/NTListItem.h>

namespace syngt {

void NTListItem::setValue(const std::string& value) {
    m_value = value;
    // TODO: Распарсить value в дерево m_root
    // Это будет делать Parser
}

void NTListItem::setValueFromRoot() {
    if (m_root) {
        SelectionMask emptyMask;
        m_value = m_root->toString(emptyMask, false);
    }
}

void NTListItem::setStringFromRoot() {
    setValueFromRoot();
}

void NTListItem::setRootFromValue() {
    // TODO: Парсинг value → root
    // Будет реализовано с Parser
}

}