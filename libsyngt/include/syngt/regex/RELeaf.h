#pragma once
#include <syngt/regex/RETree.h>
#include <string>

namespace syngt {

/**
 * @brief Базовый класс для листьев дерева RE
 * 
 * Листья - это конечные элементы: Terminal, Semantic, NonTerminal, Macro.
 * Не имеют потомков (left/right всегда nullptr).
 */
class RELeaf : public RETree {
protected:
    int m_id = 0;  // ID элемента в соответствующем списке
    
    /**
     * @brief Получить имя по ID (переопределяют наследники)
     */
    virtual std::string getNameFromID() const = 0;
    
    /**
     * @brief Установить ID по имени (переопределяют наследники)
     */
    virtual void setNameFromID(const std::string& name) = 0;
    
public:
    RELeaf() = default;
    virtual ~RELeaf() = default;
    
    void substituteAllEmpty() override {
        // Листья не имеют дочерних узлов - ничего не делаем
    }
    
    void unmarkAll() override {
        // Листья не имеют дочерних узлов - ничего не делаем
    }
    
    void save() override;
    
    std::string toString(const SelectionMask& mask, bool reverse) const override {
        (void)mask;
        (void)reverse;
        return getNameFromID();
    }
    
    RETree* left() const override { return nullptr; }
    RETree* right() const override { return nullptr; }
    
    int id() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    std::string nameID() const { return getNameFromID(); }
    void setNameID(const std::string& name) { setNameFromID(name); }

    int getOperationCount() const override {
        return 1;  // Листья считаются как одна операция
    }

};

}