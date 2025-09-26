#pragma once
#include <syngt/regex/REBinaryOp.h>

namespace syngt {

/**
 * @brief Операция итерации (повторения)
 * 
 * Представляет повторение: A * B (A повторяется, затем B)
 * В грамматиках: @* означает 0 или более раз
 * Символ операции: '*'
 * 
 * Соответствует Pascal: TRE_Iteration
 */
class REIteration : public REBinaryOp {
protected:
    char getOperationChar() const override {
        return '*';
    }
    
public:
    REIteration() = default;
    
    REIteration(std::unique_ptr<RETree> first, std::unique_ptr<RETree> second) {
        m_first = std::move(first);
        m_second = std::move(second);
    }
    
    ~REIteration() override = default;
    
    std::unique_ptr<RETree> copy() const override {
        auto first = m_first ? m_first->copy() : nullptr;
        auto second = m_second ? m_second->copy() : nullptr;
        return std::make_unique<REIteration>(std::move(first), std::move(second));
    }
    
    void tryToSetEmptyMark() override {
        if (m_first) m_first->tryToSetEmptyMark();
    }
    
    std::string toString(const SelectionMask& mask, bool reverse) const override {
        if (!m_first || !m_second) {
            return "";
        }
        
        return m_first->toString(mask, reverse) + '*' + 
               m_second->toString(mask, reverse);
    }
    
    static std::unique_ptr<REIteration> make(std::unique_ptr<RETree> first,
                                              std::unique_ptr<RETree> second) {
        return std::make_unique<REIteration>(std::move(first), std::move(second));
    }
};

}