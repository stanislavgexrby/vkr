#pragma once
#include <syngt/regex/RELeaf.h>

namespace syngt {

class Grammar;

/**
 * @brief Семантическое действие
 */
class RESemantic : public RELeaf {
private:
    Grammar* m_grammar = nullptr;
    
protected:
    std::string getNameFromID() const override;
    void setNameFromID(const std::string& name) override;
    
public:
    RESemantic() = default;
    explicit RESemantic(Grammar* grammar, int id)
        : m_grammar(grammar)
    {
        m_id = id;
    }
    
    ~RESemantic() override = default;
    
    std::unique_ptr<RETree> copy() const override;
    
    void tryToSetEmptyMark() override {
        // Семантики могут быть пустыми (epsilon-переходы)
    }
    
    static std::unique_ptr<RESemantic> makeFromID(Grammar* grammar, int id) {
        return std::make_unique<RESemantic>(grammar, id);
    }
    
    void setGrammar(Grammar* grammar) { m_grammar = grammar; }
};

}