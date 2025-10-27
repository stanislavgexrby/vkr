#pragma once
#include <syngt/regex/REBinaryOp.h>

namespace syngt {

/**
 * @brief Операция альтернативы (OR)
 * 
 * Представляет выбор: A ; B (A или B)
 * Символ операции: ';'
 */
class REOr : public REBinaryOp {
protected:
    char getOperationChar() const override {
        return ';';
    }
    
public:
    REOr() = default;
    
    REOr(std::unique_ptr<RETree> first, std::unique_ptr<RETree> second) {
        m_first = std::move(first);
        m_second = std::move(second);
    }
    
    ~REOr() override = default;
    
    std::unique_ptr<RETree> copy() const override {
        auto first = m_first ? m_first->copy() : nullptr;
        auto second = m_second ? m_second->copy() : nullptr;
        return std::make_unique<REOr>(std::move(first), std::move(second));
    }
    
    void tryToSetEmptyMark() override {
        if (m_first) m_first->tryToSetEmptyMark();
        if (m_second) m_second->tryToSetEmptyMark();
    }
    
    static std::unique_ptr<REOr> make(std::unique_ptr<RETree> first, 
                                       std::unique_ptr<RETree> second) {
        return std::make_unique<REOr>(std::move(first), std::move(second));
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