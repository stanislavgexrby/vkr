#pragma once
#include <syngt/regex/RETree.h>
#include <memory>

namespace syngt {

/**
 * @brief Базовый класс для бинарных операций
 * 
 * Бинарные операции: Or (|), And (,), Iteration (*).
 * Имеют двух потомков: first и second.
 */
class REBinaryOp : public RETree {
protected:
    std::unique_ptr<RETree> m_first;
    std::unique_ptr<RETree> m_second;
    
    /**
     * @brief Получить символ операции (переопределяют наследники)
     * @return ',' для And, ';' для Or, '*' для Iteration
     */
    virtual char getOperationChar() const = 0;
    
public:
    REBinaryOp() = default;
    virtual ~REBinaryOp() = default;
    
    void substituteAllEmpty() override {
        if (m_first) m_first->substituteAllEmpty();
        if (m_second) m_second->substituteAllEmpty();
    }
    
    void unmarkAll() override {
        if (m_first) m_first->unmarkAll();
        if (m_second) m_second->unmarkAll();
    }
    
    void save() override;
    
    bool allMacroWasOpened() const override {
        bool firstOk = m_first ? m_first->allMacroWasOpened() : true;
        bool secondOk = m_second ? m_second->allMacroWasOpened() : true;
        return firstOk && secondOk;
    }
    
    bool allDefinitionWasClosed() const override {
        bool firstOk = m_first ? m_first->allDefinitionWasClosed() : true;
        bool secondOk = m_second ? m_second->allDefinitionWasClosed() : true;
        return firstOk && secondOk;
    }
    
    std::string toString(const SelectionMask& mask, bool reverse) const override {
        if (!m_first || !m_second) {
            return "";
        }
        
        if (reverse) {
            return m_second->toString(mask, reverse) + 
                   getOperationChar() + 
                   m_first->toString(mask, reverse);
        } else {
            return m_first->toString(mask, reverse) + 
                   getOperationChar() + 
                   m_second->toString(mask, reverse);
        }
    }
    
    RETree* left() const override { return m_first.get(); }
    RETree* right() const override { return m_second.get(); }
    
    void setFirst(std::unique_ptr<RETree> first) {
        m_first = std::move(first);
    }
    
    void setSecond(std::unique_ptr<RETree> second) {
        m_second = std::move(second);
    }
    
    char operationChar() const { return getOperationChar(); }
    const RETree* firstOperand() const { return m_first.get(); }
    const RETree* secondOperand() const { return m_second.get(); }
};

}