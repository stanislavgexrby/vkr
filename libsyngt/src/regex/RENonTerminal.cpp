#include <syngt/regex/RENonTerminal.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/core/Types.h>
#include <stdexcept>

namespace syngt {

std::string RENonTerminal::getNameFromID() const {
    if (!m_grammar) {
        return "<no grammar>";
    }
    
    try {
        return m_grammar->nonTerminals()->getString(m_id);
    } catch (...) {
        return "<error>";
    }
}

void RENonTerminal::setNameFromID(const std::string& name) {
    if (!m_grammar) {
        throw std::runtime_error("Grammar not set");
    }
    
    int id = m_grammar->findNonTerminal(name);
    if (id == -1) {
        id = m_grammar->addNonTerminal(name);
    }
    m_id = id;
}

RETree* RENonTerminal::getRoot() const {
    NTListItem* item = getListItem();
    return item ? item->root() : nullptr;
}

NTListItem* RENonTerminal::getListItem() const {
    if (!m_grammar) {
        return nullptr;
    }
    
    std::string ntName = m_grammar->nonTerminals()->getString(m_id);
    
    return m_grammar->getNTItem(ntName);
}

std::unique_ptr<RETree> RENonTerminal::copy() const {
    return std::make_unique<RENonTerminal>(m_grammar, m_id, m_isOpen);
}

std::string RENonTerminal::toString(const SelectionMask& mask, bool reverse) const {
    if (m_drawObj >= 0 && !mask.empty()) {
        for (int id : mask) {
            if (id == m_drawObj) {
                RETree* root = getRoot();
                if (root) {
                    SelectionMask emptyMask;
                    return "(" + root->toString(emptyMask, reverse) + ")";
                }
                break;
            }
        }
    }
    return getNameFromID();
}

bool RENonTerminal::allMacroWasOpened() const {
    NTListItem* item = getListItem();
    if (!item) return true;

    if (item->mark() == cmInProgress) {
        throw std::runtime_error("Circular macro reference detected: " + getNameFromID());
    }

    if (!item->isMacro()) return true;

    int prevMark = item->mark();
    item->setMark(cmInProgress);
    m_isOpen = true;

    RETree* root = getRoot();
    bool result = root ? root->allMacroWasOpened() : true;

    item->setMark(prevMark);
    return result;
}

}