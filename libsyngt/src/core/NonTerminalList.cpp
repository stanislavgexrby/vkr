#include <syngt/core/NonTerminalList.h>
#include <syngt/core/Grammar.h>
#include <syngt/regex/RETree.h>

namespace syngt {

void NonTerminalList::fillNew() {
    m_list.clear();
    m_items.clear();
    
    m_list.push_back("S");
    
    auto item = std::make_unique<NTListItem>(m_grammar, "S");
    m_items.push_back(std::move(item));
}

int NonTerminalList::add(const std::string& s) {
    int existingIndex = find(s);
    if (existingIndex >= 0) {
        return existingIndex;
    }
    
    m_list.push_back(s);
    
    auto item = std::make_unique<NTListItem>(m_grammar, s);
    m_items.push_back(std::move(item));
    
    return static_cast<int>(m_list.size()) - 1;
}

void NonTerminalList::clear() {
    m_list.clear();
    m_items.clear();
}

void NonTerminalList::setGrammar(Grammar* grammar) {
    m_grammar = grammar;
    for (auto& item : m_items) {
        if (item) {
            item->setGrammar(grammar);
        }
    }
}

void NonTerminalList::setRoot(int index, std::unique_ptr<RETree> root) {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        m_items[index]->setRoot(std::move(root));
    }
}

void NonTerminalList::setRootByName(const std::string& name, std::unique_ptr<RETree> root) {
    int index = find(name);
    if (index >= 0) {
        setRoot(index, std::move(root));
    }
}

}