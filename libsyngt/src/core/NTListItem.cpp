#include <syngt/core/NTListItem.h>
#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>
#include <syngt/regex/RETree.h>
#include <iostream>

namespace syngt {

NTListItem::NTListItem() = default;

NTListItem::NTListItem(Grammar* grammar, const std::string& name)
    : m_grammar(grammar)
    , m_name(name)
{}

NTListItem::~NTListItem() = default;

NTListItem::NTListItem(NTListItem&&) noexcept = default;
NTListItem& NTListItem::operator=(NTListItem&&) noexcept = default;

void NTListItem::setValue(const std::string& value) {
    m_value = value;
    setRootFromValue();
}

void NTListItem::setRoot(std::unique_ptr<RETree> root) {
    m_root = std::move(root);
    setValueFromRoot();
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
    if (m_value.empty()) {
        m_root.reset();
        return;
    }
    
    if (m_grammar) {
        try {
            Parser parser;
            m_root = parser.parse(m_value, m_grammar);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse rule for '" << m_name << "': " << e.what() << "\n";
            std::cerr << "  Rule was: " << m_value << "\n";
            throw;
        }
    }
}

void NTListItem::updateValueFromRoot() {
    setValueFromRoot();
}

std::unique_ptr<RETree> NTListItem::copyRETree() const {
    if (m_root) {
        return m_root->copy();
    }
    return nullptr;
}

}