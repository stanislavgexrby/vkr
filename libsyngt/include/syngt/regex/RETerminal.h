#pragma once
#include <syngt/regex/RELeaf.h>

namespace syngt {

class Grammar;

/**
 * @brief Терминальный символ грамматики
 */
class RETerminal : public RELeaf {
private:
    Grammar* m_grammar = nullptr;
    
protected:
    std::string getNameFromID() const override;
    void setNameFromID(const std::string& name) override;
    
public:
    RETerminal() = default;
    explicit RETerminal(Grammar* grammar, int id) 
        : m_grammar(grammar) 
    {
        m_id = id;
    }
    
    ~RETerminal() override = default;
    
    std::unique_ptr<RETree> copy() const override;
    std::string toString(const SelectionMask& mask, bool reverse) const override;
    
    void tryToSetEmptyMark() override {
        // Терминалы не могут быть пустыми
    }
    
    static std::unique_ptr<RETerminal> makeFromID(Grammar* grammar, int id) {
        return std::make_unique<RETerminal>(grammar, id);
    }
    
    void setGrammar(Grammar* grammar) { m_grammar = grammar; }
};

}