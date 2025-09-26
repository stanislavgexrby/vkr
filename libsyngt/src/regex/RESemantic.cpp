#include <syngt/regex/RESemantic.h>
#include <syngt/core/Grammar.h>
#include <stdexcept>

namespace syngt {

std::string RESemantic::getNameFromID() const {
    if (!m_grammar) {
        return "<no grammar>";
    }
    
    try {
        return m_grammar->semantics()->getString(m_id);
    } catch (...) {
        return "<error>";
    }
}

void RESemantic::setNameFromID(const std::string& name) {
    if (!m_grammar) {
        throw std::runtime_error("Grammar not set");
    }
    
    int id = m_grammar->findSemantic(name);
    if (id == -1) {
        id = m_grammar->addSemantic(name);
    }
    m_id = id;
}

std::unique_ptr<RETree> RESemantic::copy() const {
    return std::make_unique<RESemantic>(m_grammar, m_id);
}

}