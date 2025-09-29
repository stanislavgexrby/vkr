#pragma once
#include <syngt/regex/REMacro.h>

namespace syngt {

class Grammar;
class NTListItem;

/**
 * @brief Нетерминальный символ грамматики
 */
class RENonTerminal : public REMacro {
private:
    Grammar* m_grammar = nullptr;
    bool m_isOpen = false;
    
protected:
    std::string getNameFromID() const override;
    void setNameFromID(const std::string& name) override;
    
    RETree* getRoot() const override;
    NTListItem* getListItem() const override;
    
public:
    RENonTerminal() = default;
    
    explicit RENonTerminal(Grammar* grammar, int id, bool open = false)
        : m_grammar(grammar)
        , m_isOpen(open)
    {
        m_id = id;
    }
    
    ~RENonTerminal() override = default;
    
    std::unique_ptr<RETree> copy() const override;
    
    bool allMacroWasOpened() const override;
    
    void tryToSetEmptyMark() override {
        // TODO: логика установки пустой отметки для нетерминалов
    }
    
    static std::unique_ptr<RENonTerminal> makeFromID(Grammar* grammar, int id) {
        return std::make_unique<RENonTerminal>(grammar, id, false);
    }
    
    static std::unique_ptr<RENonTerminal> makeFromIDAndOpen(Grammar* grammar, int id, bool open) {
        return std::make_unique<RENonTerminal>(grammar, id, open);
    }
    
    bool isOpen() const { return m_isOpen; }
    void setOpen(bool open) { m_isOpen = open; }
    void setGrammar(Grammar* grammar) { m_grammar = grammar; }
    /**
     * @brief Получить индекс нетерминала
     */
    int getID() const { return m_id; }
    
    Grammar* grammar() const { return m_grammar; }
};

}