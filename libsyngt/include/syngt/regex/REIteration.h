#pragma once
#include <syngt/regex/REBinaryOp.h>
#include <syngt/regex/REOr.h>

namespace syngt {

/**
 * @brief Операция итерации (повторения)
 * 
 * Представляет повторение: A * B (A повторяется, затем B)
 * В грамматиках: @* означает 0 или более раз
 * Символ операции: '*'
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
        
        std::string result = m_first->toString(mask, reverse) + '*';
        
        if (dynamic_cast<const REOr*>(m_second.get())) {
            result += '(' + m_second->toString(mask, reverse) + ')';
        } else {
            result += m_second->toString(mask, reverse);
        }
        
        return result;
    }
    
    static std::unique_ptr<REIteration> make(std::unique_ptr<RETree> first,
                                              std::unique_ptr<RETree> second) {
        return std::make_unique<REIteration>(std::move(first), std::move(second));
    }

    syngt::graphics::DrawObject* drawObjectsToRight(
        syngt::graphics::DrawObjectList* list,
        syngt::SemanticIDList*& semantics,
        syngt::graphics::DrawObject* fromDO,
        int ward,
        int& height
    ) const override;

};

}