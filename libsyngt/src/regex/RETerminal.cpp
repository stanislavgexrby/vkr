#include <syngt/regex/RETerminal.h>
#include <syngt/core/Grammar.h>
#include <stdexcept>

namespace syngt {

std::string RETerminal::getNameFromID() const {
    if (!m_grammar) {
        return "<no grammar>";
    }
    
    try {
        return m_grammar->terminals()->getString(m_id);
    } catch (...) {
        return "<error>";
    }
}

void RETerminal::setNameFromID(const std::string& name) {
    if (!m_grammar) {
        throw std::runtime_error("Grammar not set");
    }
    
    int id = m_grammar->findTerminal(name);
    if (id == -1) {
        id = m_grammar->addTerminal(name);
    }
    m_id = id;
}

std::unique_ptr<RETree> RETerminal::copy() const {
    return std::make_unique<RETerminal>(m_grammar, m_id);
}

}