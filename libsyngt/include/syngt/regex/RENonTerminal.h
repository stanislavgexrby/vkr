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

protected:
    std::string getNameFromID() const override;
    void setNameFromID(const std::string& name) override;
    
    RETree* getRoot() const override;
    NTListItem* getListItem() const override;
    
public:
    RENonTerminal() = default;
    
    explicit RENonTerminal(Grammar* grammar, int id, bool open = false)
        : m_grammar(grammar)
    {
        m_id = id;
        m_isOpen = open;
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
    
    void setGrammar(Grammar* grammar) { m_grammar = grammar; }
    /**
     * @brief Получить индекс нетерминала
     */
    int getID() const { return m_id; }
    
    Grammar* grammar() const { return m_grammar; }

    std::string toString(const SelectionMask& mask, bool reverse) const override;

    syngt::graphics::DrawObject* drawObjectsToRight(
        syngt::graphics::DrawObjectList* list,
        syngt::SemanticIDList*& semantics,
        syngt::graphics::DrawObject* fromDO,
        int ward,
        int& height
    ) const override;

};

}