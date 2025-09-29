#pragma once
#include <syngt/core/Types.h>
#include <memory>
#include <string>

namespace syngt {

class Grammar;
class Parser;
class RETree;

/**
 * @brief Элемент нетерминала
 */
class NTListItem {
private:
    Grammar* m_grammar = nullptr;
    std::string m_name;
    std::string m_value;
    std::unique_ptr<RETree> m_root;
    int m_mark = cmNotMarked;
    
    void setValueFromRoot();
    void setStringFromRoot();
    void setRootFromValue();
    
public:
    NTListItem();
    explicit NTListItem(Grammar* grammar, const std::string& name);
    ~NTListItem();
    
    NTListItem(const NTListItem&) = delete;
    NTListItem& operator=(const NTListItem&) = delete;
    NTListItem(NTListItem&&) noexcept;
    NTListItem& operator=(NTListItem&&) noexcept;
    
    void setValue(const std::string& value);
    void setRoot(std::unique_ptr<RETree> root);
    
    /**
     * @brief Создать копию дерева RE
     * Реализация в .cpp (требует полный тип RETree)
     */
    std::unique_ptr<RETree> copyRETree() const;
    
    const std::string& name() const { return m_name; }
    const std::string& value() const { return m_value; }
    RETree* root() const { return m_root.get(); }
    int mark() const { return m_mark; }
    Grammar* grammar() const { return m_grammar; }
    
    void setName(const std::string& name) { m_name = name; }
    void setMark(int mark) { m_mark = mark; }
    void setGrammar(Grammar* grammar) { m_grammar = grammar; }
    
    bool hasRoot() const { return m_root != nullptr; }
    void updateValueFromRoot();
};

}