#pragma once
#include <syngt/regex/REBinaryOp.h>

namespace syngt {

/**
 * @brief Операция последовательности (AND)
 * 
 * Представляет последовательность: A , B (сначала A, потом B)
 * Символ операции: ','
 * 
 * Соответствует Pascal: TRE_And
 */
class REAnd : public REBinaryOp {
protected:
    char getOperationChar() const override {
        return ',';
    }
    
public:
    REAnd() = default;
    
    REAnd(std::unique_ptr<RETree> first, std::unique_ptr<RETree> second) {
        m_first = std::move(first);
        m_second = std::move(second);
    }
    
    ~REAnd() override = default;
    
    std::unique_ptr<RETree> copy() const override {
        auto first = m_first ? m_first->copy() : nullptr;
        auto second = m_second ? m_second->copy() : nullptr;
        return std::make_unique<REAnd>(std::move(first), std::move(second));
    }
    
    void tryToSetEmptyMark() override {
        if (m_first) m_first->tryToSetEmptyMark();
        if (m_second) m_second->tryToSetEmptyMark();
    }
    
    static std::unique_ptr<REAnd> make(std::unique_ptr<RETree> first,
                                        std::unique_ptr<RETree> second) {
        return std::make_unique<REAnd>(std::move(first), std::move(second));
    }
};

}